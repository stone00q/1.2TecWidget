#include "TecplotWidget.h"
#include <QtDebug>
#include <QElapsedTimer>//cout runtime
// VTK 核心基础模块
#include <vtkSmartPointer.h>
#include <vtkInformation.h>
#include <vtkErrorCode.h>
#include <vtkAutoInit.h>
#include <vtkMath.h>
#include <vtkProbeFilter.h>
// VTK 数据模型
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkIdTypeArray.h>

// VTK 可视化/渲染
#include <vtkTextProperty.h>
#include <vtkInteractorStyleTrackballCamera.h>

// VTK 数据处理/算法
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkStripper.h>
#include <vtkFeatureEdges.h>
#include <vtkRotationalExtrusionFilter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkGeometryFilter.h>
#include <vtkBoxClipDataSet.h>
#include <vtkMergePoints.h>
#include <vtkTransformFilter.h>

//S1提取
#include <vtkImplicitBoolean.h>
#include <vtkClipDataSet.h>
#include <vtkConnectivityFilter.h>
// C++ 标准库
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
//VTK_MODULE_INIT(vtkRenderingOpenGL2);
//VTK_MODULE_INIT(vtkInteractionStyle);
////VTK_MODULE_INIT(vtkRenderingContextOpenGL2);
//VTK_MODULE_INIT(vtkRenderingFreeType)



/***********S1S2面*********************/
namespace  {
    /**
     * Calculate the theta bounds for rotational extrusion based on the unstructured grid's bounds.
     * Returns a pair of doubles representing the minimum and maximum theta angles in degrees.
     */
    std::pair<double, double> calculateThetaBounds(vtkUnstructuredGrid* ugrid) {
        double bounds[6];
        ugrid->GetBounds(bounds);
        double xmin = bounds[0], xmax = bounds[1];
        double ymin = bounds[2], ymax = bounds[3];
        double zmin = bounds[4], zmax = bounds[5];

        // Check if the YZ projection contains the origin
        bool yzContainsOrigin = (ymin <= 0 && 0 <= ymax) && (zmin <= 0 && 0 <= zmax);
        if (yzContainsOrigin) {
            return { -180.0, 180.0 }; // Full circle in degrees
        }

        // Generate the eight vertices of the bounding box
        std::vector<std::array<double, 3>> vertices;
        for (double x : {xmin, xmax}) {
            for (double y : {ymin, ymax}) {
                for (double z : {zmin, zmax}) {
                    vertices.push_back(std::array<double, 3>({ x, y, z }));
                }
            }
        }

        // Calculate theta for each vertex
        std::vector<double> thetas;
        for (const auto& v : vertices) {
            double y = v[1];
            double z = v[2];
            double theta;
            if (y == 0 && z == 0) {
                theta = 0.0; // On X-axis, set theta to 0
            }
            else {
                theta = std::atan2(z, y) * (180.0 / 3.14159265358979); // Convert radians to degrees
            }
            thetas.push_back(theta);
        }

        // Find minimum and maximum theta
        double minTheta = *std::min_element(thetas.begin(), thetas.end());
        double maxTheta = *std::max_element(thetas.begin(), thetas.end());

        return { minTheta, maxTheta };
    }

    /**
     * Extract and process a surface from an unstructured grid file.
     * Parameters:
     * - blockFileName: Input file name (e.g., "e3-stator.vtu")
     * - preprocessingFlag: Whether to preprocess the grid to extract periodic faces
     * - relativeR: Relative radial height for curve generation (default 50.0)
     * Returns: Processed vtkPolyData object
     */
    /**子午面投影**/
    vtkSmartPointer<vtkPolyData> meridionalProjection(const std::vector<vtkSmartPointer<vtkPolyData>>& inputs) {
        // Step 1: Create an append filter to merge the projected data
        auto appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

        // Step 2: Process each input surface
        for (const auto& pd : inputs) {
            // Get the original points
            auto points = pd->GetPoints();
            auto numPoints = points->GetNumberOfPoints();

            // Create new points for the projection (X, R, 0)
            auto newPoints = vtkSmartPointer<vtkPoints>::New();
            newPoints->SetNumberOfPoints(numPoints);

            for (vtkIdType i = 0; i < numPoints; ++i) {
                double p[3];
                points->GetPoint(i, p);
                double r = std::sqrt(p[1] * p[1] + p[2] * p[2]); // Radial distance
                newPoints->SetPoint(i, p[0], r, 0.0); // (X, R, 0)
            }

            // Create a new PolyData with the projected points
            auto projectedPd = vtkSmartPointer<vtkPolyData>::New();
            projectedPd->DeepCopy(pd);
            projectedPd->SetPoints(newPoints);

            // Add to the append filter
            appendFilter->AddInputData(projectedPd);
        }

        // Step 3: Merge all projected surfaces
        appendFilter->Update();
        auto merged = appendFilter->GetOutput();

        // Step 4: Clean duplicate points
        auto cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
        cleaner->SetInputData(merged);
        cleaner->SetTolerance(1e-6); // Adjust based on data scale
        cleaner->Update();

        return cleaner->GetOutput();
    }
    /**S1-step2文件*/
    // Helper function: Convert vtkPoints to std::vector<std::array<double, 3>>
    std::vector<std::array<double, 3>> vtkPointsToVector(vtkPoints* points) {
        std::vector<std::array<double, 3>> result;
        for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i) {
            double point[3];
            points->GetPoint(i, point);
            result.push_back({ point[0], point[1], point[2] });
        }
        return result;
    }

    // Helper function: Convert std::vector<std::array<double, 3>> to vtkPoints
    vtkSmartPointer<vtkPoints> vectorToVtkPoints(const std::vector<std::array<double, 3>>& points) {
        auto vtkpoints = vtkSmartPointer<vtkPoints>::New();

        for (const auto& point : points) {
            vtkpoints->InsertNextPoint(point.data());
        }
        return vtkpoints;
    }

    vtkSmartPointer<vtkPolyData> split_loop_by_angle(vtkPolyData* input_poly, double feature_angle_degrees) {
        auto points = vtkPointsToVector(input_poly->GetPoints());
        int n = points.size();
        if (n < 3) {
            return input_poly;
        }

        std::vector<double> angles(n, 0.0);
        for (int i = 0; i < n; ++i) {
            int prev_i = (i - 1 + n) % n;
            int next_i = (i + 1) % n;

            std::array<double, 3> vec_prev = {
                points[prev_i][0] - points[i][0],
                points[prev_i][1] - points[i][1],
                points[prev_i][2] - points[i][2]
            };
            std::array<double, 3> vec_next = {
                points[next_i][0] - points[i][0],
                points[next_i][1] - points[i][1],
                points[next_i][2] - points[i][2]
            };

            double dot = vec_prev[0] * vec_next[0] + vec_prev[1] * vec_next[1] + vec_prev[2] * vec_next[2];
            double norm_prev = std::sqrt(vec_prev[0] * vec_prev[0] + vec_prev[1] * vec_prev[1] + vec_prev[2] * vec_prev[2]);
            double norm_next = std::sqrt(vec_next[0] * vec_next[0] + vec_next[1] * vec_next[1] + vec_next[2] * vec_next[2]);

            if (norm_prev < 1e-6 || norm_next < 1e-6) {
                angles[i] = 0.0;
                continue;
            }

            double cos_theta = dot / (norm_prev * norm_next);
            cos_theta = std::max(-1.0, std::min(1.0, cos_theta));
            angles[i] = vtkMath::DegreesFromRadians(std::acos(cos_theta));
        }

        std::vector<int> split_points;
        for (int i = 0; i < n; ++i) {
            if (angles[i] > feature_angle_degrees) {
                split_points.push_back(i);
            }
        }
        std::sort(split_points.begin(), split_points.end());

        if (split_points.empty()) {
            return input_poly;
        }

        auto new_lines = vtkSmartPointer<vtkCellArray>::New();
        int num_splits = split_points.size();

        for (int i = 0; i < num_splits; ++i) {
            int start = split_points[i];
            int end = split_points[(i + 1) % num_splits];

            std::vector<int> indices;
            if (start <= end) {
                for (int j = start; j <= end; ++j) {
                    indices.push_back(j);
                }
            }
            else {
                for (int j = start; j < n; ++j) {
                    indices.push_back(j);
                }
                for (int j = 0; j <= end; ++j) {
                    indices.push_back(j);
                }
            }

            auto polyline = vtkSmartPointer<vtkPolyLine>::New();
            polyline->GetPointIds()->SetNumberOfIds(indices.size());
            for (size_t j = 0; j < indices.size(); ++j) {
                polyline->GetPointIds()->SetId(j, indices[j]);
            }

            new_lines->InsertNextCell(polyline);
        }

        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->SetPoints(input_poly->GetPoints());
        output->SetLines(new_lines);

        return output;
    }
    vtkSmartPointer<vtkPolyData> merge_loop_segments(vtkPolyData* input_pd) {
        // 检查输入是否有效
        if (!input_pd) {
            throw std::runtime_error("输入必须是 vtkPolyData 类型");
        }

        auto lines = input_pd->GetLines();
        vtkIdType n_cells = lines->GetNumberOfCells();
        if (n_cells < 1) {
            return input_pd; // 如果没有线段，直接返回输入
        }

        // 使用 vtkStripper 将线段连接成连续的折线
        auto stripper = vtkSmartPointer<vtkStripper>::New();
        stripper->SetInputData(input_pd);
        stripper->JoinContiguousSegmentsOn(); // 启用连接连续段的功能
        stripper->Update();

        // 获取 stripper 的输出
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(stripper->GetOutput());

        // 检查输出是否形成闭合环路（可选）
        auto output_lines = output->GetLines();
//        if (output_lines->GetNumberOfCells() == 0) {
//            throw std::runtime_error("vtkStripper 未能生成有效的折线");
//        }

        // 如果需要确保是单一闭合环路，可以进一步验证
        vtkSmartPointer<vtkIdList> id_list = vtkSmartPointer<vtkIdList>::New();
        output_lines->InitTraversal();
        output_lines->GetNextCell(id_list);
        vtkIdType num_points = id_list->GetNumberOfIds();
        if (num_points > 0 && id_list->GetId(0) != id_list->GetId(num_points - 1)) {
            throw std::runtime_error("输出不是闭合环路");
        }

        // 遍历点序列，跳过连续重复点
        vtkNew<vtkPoints> unique_points;
        vtkNew<vtkIdList> unique_ids;
        double prev_point[3] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };  // 初始值设为无效
        for (vtkIdType i = 0; i < id_list->GetNumberOfIds(); ++i) {
            vtkIdType pid = id_list->GetId(i);
            double current_point[3];
            output->GetPoint(pid, current_point);

            // 检查当前点是否与前一点坐标相同
            if (i == 0 || vtkMath::Distance2BetweenPoints(prev_point, current_point) > 1e-12) {
                vtkIdType new_pid = unique_points->InsertNextPoint(current_point);
                unique_ids->InsertNextId(new_pid);
                prev_point[0] = current_point[0];
                prev_point[1] = current_point[1];
                prev_point[2] = current_point[2];
            }
        }

        // 构建最终无重复点的线段
        vtkNew<vtkCellArray> final_lines;
        final_lines->InsertNextCell(unique_ids);

        auto merged_pd = vtkSmartPointer<vtkPolyData>::New();
        merged_pd->SetPoints(unique_points);
        merged_pd->SetLines(final_lines);

        return merged_pd;
    }
    struct PointWithIndex {
        std::array<double, 3> point;
        size_t index;
    };

    std::pair<std::vector<std::array<double, 3>>, std::vector<size_t>> sort_trapezoid_points_with_index(
        const std::vector<PointWithIndex>& indexed_points) {

        if (indexed_points.size() != 4) {
            throw std::runtime_error("Must input 4 points");
        }

        // Sort by x coordinate
        std::vector<PointWithIndex> sorted_by_x = indexed_points;
        std::sort(sorted_by_x.begin(), sorted_by_x.end(),
            [](const PointWithIndex& a, const PointWithIndex& b) {
                return a.point[0] < b.point[0];
            });

        // Separate left and right boundaries
        std::vector<PointWithIndex> left_points(sorted_by_x.begin(), sorted_by_x.begin() + 2);
        std::vector<PointWithIndex> right_points(sorted_by_x.begin() + 2, sorted_by_x.end());

        // Sort left boundary by y coordinate ascending
        std::sort(left_points.begin(), left_points.end(),
            [](const PointWithIndex& a, const PointWithIndex& b) {
                return a.point[1] < b.point[1];
            });

        // Sort right boundary by y coordinate descending
        std::sort(right_points.begin(), right_points.end(),
            [](const PointWithIndex& a, const PointWithIndex& b) {
                return a.point[1] > b.point[1];
            });

        // Assemble results
        std::vector<std::array<double, 3>> sorted_points = {
            left_points[0].point,  // Bottom left point
            left_points[1].point,  // Top left point
            right_points[0].point, // Top right point
            right_points[1].point  // Bottom right point
        };

        std::vector<size_t> original_indices = {
            left_points[0].index,  // Bottom left point index
            left_points[1].index,  // Top left point index
            right_points[0].index, // Top right point index
            right_points[1].index  // Bottom right point index
        };

        return { sorted_points, original_indices };
    }

    std::vector<double> get_y_from(const std::vector<std::array<double, 3>>& points, const std::vector<double>& x_query) {
        std::vector<double> yout(x_query.size(), 0.0);

        // Extract x and y coordinates
        std::vector<double> x(points.size());
        std::vector<double> y(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            x[i] = points[i][0];
            y[i] = points[i][1];
        }

        // Traverse query points
        for (size_t j = 0; j < x_query.size(); ++j) {
            // Handle boundary cases
            if (x_query[j] <= x[0]) {
                yout[j] = y[0];
                continue;
            }
            else if (x_query[j] >= x.back()) {
                yout[j] = y.back();
                continue;
            }

            // Find segment containing query point
            size_t i = 0;
            while (i < x.size() - 1) {
                double x0 = x[i], y0 = y[i];
                double x1 = x[i + 1], y1 = y[i + 1];

                double x_min = std::min(x0, x1);
                double x_max = std::max(x0, x1);

                if (x_query[j] < x_min || x_query[j] > x_max) {
                    ++i;
                    continue;
                }

                // Handle vertical segments
                if (std::abs(x0 - x1) < 1e-6) {
                    yout[j] = (y0 + y1) / 2;  // Take midpoint Y value
                    break;
                }

                // Linear interpolation
                double t = (x_query[j] - x0) / (x1 - x0);
                yout[j] = y0 + t * (y1 - y0);
                break;
            }
        }

        return yout;
    }

    std::tuple<vtkSmartPointer<vtkPolyData>, std::vector<std::vector<int>>, std::vector<vtkIdType>>
    segment_polyline_by_feature_angle(vtkPolyData* polydata, double featureAngle) {
        auto points = polydata->GetPoints();
        auto cell = polydata->GetCell(0);  // Assume only one PolyLine cell
        vtkIdType n_points = cell->GetNumberOfPoints();

        if (n_points < 4) {
            std::vector<vtkIdType> point_ids(n_points);
            for (vtkIdType k = 0; k < n_points; ++k) {
                point_ids[k] = cell->GetPointId(k);
            }
            std::vector<std::vector<int>> empty_vector;
            return std::make_tuple(polydata, empty_vector, point_ids);
        }

        vtkIdType n_distinct = n_points - 1;
        std::vector<vtkIdType> point_ids(n_distinct);
        for (vtkIdType k = 0; k < n_distinct; ++k) {
            point_ids[k] = cell->GetPointId(k);
        }

        double featureAngle_rad = vtkMath::RadiansFromDegrees(featureAngle);
        double cos_feature = std::cos(featureAngle_rad);

        std::vector<vtkIdType> feature_k;
        for (vtkIdType k = 0; k < n_distinct; ++k) {
            vtkIdType prev_k = (k - 1 + n_distinct) % n_distinct;
            vtkIdType next_k = (k + 1) % n_distinct;

            double p[3], p_prev[3], p_next[3];
            points->GetPoint(point_ids[k], p);
            points->GetPoint(point_ids[prev_k], p_prev);
            points->GetPoint(point_ids[next_k], p_next);

            double u[3] = { p[0] - p_prev[0], p[1] - p_prev[1], p[2] - p_prev[2] };
            double v[3] = { p_next[0] - p[0], p_next[1] - p[1], p_next[2] - p[2] };
            double norm_u = vtkMath::Norm(u);
            double norm_v = vtkMath::Norm(v);

            if (norm_u < 1e-6 || norm_v < 1e-6) {
                continue;
            }

            double cos_theta = vtkMath::Dot(u, v) / (norm_u * norm_v);
            cos_theta = std::max(-1.0, std::min(1.0, cos_theta));

            if (cos_theta < cos_feature) {
                feature_k.push_back(k);
            }
        }

        assert(feature_k.size() == 4 && "Expected 4 feature points");

        std::vector<PointWithIndex> cords;
        for (vtkIdType k : feature_k) {
            double p[3];
            points->GetPoint(point_ids[k], p);
            cords.push_back({ {p[0], p[1], p[2]}, static_cast<size_t>(point_ids[k]) });
        }

        //auto [sorted_pts, sorted_pts_id_size_t] = sort_trapezoid_points_with_index(cords);
        auto result = sort_trapezoid_points_with_index(cords);
        auto& sorted_pts = result.first;
        auto& sorted_pts_id_size_t = result.second;
        std::vector<vtkIdType> sorted_pts_id(sorted_pts_id_size_t.begin(), sorted_pts_id_size_t.end());

        std::map<std::pair<vtkIdType, vtkIdType>, std::vector<int>> segment_dict;
        for (int i = 0; i < 4; ++i) {
            vtkIdType line_pid_start = sorted_pts_id[i];
            vtkIdType line_pid_end = sorted_pts_id[(i + 1) % 4];
            auto key = std::make_pair(std::min(line_pid_start, line_pid_end), std::max(line_pid_start, line_pid_end));
            segment_dict[key].push_back(i);
        }

        std::vector<std::vector<int>> sorted_lns;
        auto seglines = vtkSmartPointer<vtkCellArray>::New();
        size_t m = feature_k.size();

        for (size_t i = 0; i < m; ++i) {
            vtkIdType start_k = feature_k[i];
            vtkIdType end_k = feature_k[(i + 1) % m];
            std::vector<vtkIdType> segment_k;

            if (start_k <= end_k) {
                for (vtkIdType k = start_k; k <= end_k; ++k) {
                    segment_k.push_back(k);
                }
            }
            else {
                for (vtkIdType k = start_k; k < n_distinct; ++k) {
                    segment_k.push_back(k);
                }
                for (vtkIdType k = 0; k <= end_k; ++k) {
                    segment_k.push_back(k);
                }
            }

            std::vector<vtkIdType> segment_pids;
            for (vtkIdType k : segment_k) {
                segment_pids.push_back(point_ids[k]);
            }

            vtkIdType line_pid_start = point_ids[start_k];
            vtkIdType line_pid_end = point_ids[end_k];
            auto key = std::make_pair(std::min(line_pid_start, line_pid_end), std::max(line_pid_start, line_pid_end));
            auto linePos = segment_dict[key];
            sorted_lns.push_back(linePos);

            auto new_line = vtkSmartPointer<vtkPolyLine>::New();
            new_line->GetPointIds()->SetNumberOfIds(segment_pids.size());
            for (size_t i = 0; i < segment_pids.size(); ++i) {
                new_line->GetPointIds()->SetId(i, segment_pids[i]);
            }
            seglines->InsertNextCell(new_line);
        }

        auto polydataSegments = vtkSmartPointer<vtkPolyData>::New();
        polydataSegments->SetPoints(polydata->GetPoints());
        polydataSegments->SetLines(seglines);

        return std::make_tuple(polydataSegments, sorted_lns, sorted_pts_id);
    }

    vtkSmartPointer<vtkPolyData> numpy_to_vtk_polyline(const std::vector<std::array<double, 3>>& points) {
        auto vtk_points = vtkSmartPointer<vtkPoints>::New();
        for (const auto& p : points) {
            vtk_points->InsertNextPoint(p.data());
        }

        auto poly_line = vtkSmartPointer<vtkPolyLine>::New();
        poly_line->GetPointIds()->SetNumberOfIds(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            poly_line->GetPointIds()->SetId(i, i);
        }

        auto cells = vtkSmartPointer<vtkCellArray>::New();
        cells->InsertNextCell(poly_line);

        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->SetPoints(vtk_points);
        output->SetLines(cells);

        return output;
    }

    vtkSmartPointer<vtkPolyData> generate_radial_curve(vtkPolyData* projected_surface, double relative_R, int num_samples = 10) {
        if (relative_R < 0 || relative_R > 100) {
            throw std::runtime_error("relative_R must be in [0, 100]");
        }

        auto points = projected_surface->GetPoints();
        auto coords = vtkPointsToVector(points);//！！调用函数

        std::vector<double> R_values(coords.size());
        std::vector<double> X_values(coords.size());
        for (size_t i = 0; i < coords.size(); ++i) {
            R_values[i] = coords[i][1];
            X_values[i] = coords[i][0];
        }

        double R_min = *std::min_element(R_values.begin(), R_values.end());
        double R_max = *std::max_element(R_values.begin(), R_values.end());
        double X_min = *std::min_element(X_values.begin(), X_values.end());
        double X_max = *std::max_element(X_values.begin(), X_values.end());

        auto featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
        featureEdges->SetInputData(projected_surface);
        featureEdges->BoundaryEdgesOn();
        featureEdges->FeatureEdgesOn();
        featureEdges->ManifoldEdgesOff();
        featureEdges->NonManifoldEdgesOff();
        featureEdges->Update();

        auto outline = featureEdges->GetOutput();
        /////！！！！！调用函数
        auto merged_outline = merge_loop_segments(outline);

        ///！！！调用函数
        vtkSmartPointer<vtkPolyData> splitedOutline;
        std::vector<std::vector<int>> sorted_lns;
        std::vector<vtkIdType> sorted_pts_id;
        std::tie(splitedOutline, sorted_lns, sorted_pts_id) = segment_polyline_by_feature_angle(merged_outline, 60);

        std::vector<double> outcurveX(num_samples);
        for (int i = 0; i < num_samples; ++i) {
            outcurveX[i] = X_min + (X_max - X_min) * i / (num_samples - 1);
        }

        auto lines = splitedOutline->GetLines();
        lines->InitTraversal();

        vtkSmartPointer<vtkIdList> id_list = vtkSmartPointer<vtkIdList>::New();
        for (int i = 0; i < sorted_lns[1][0]; ++i) {
            lines->GetNextCell(id_list);
        }

        vtkSmartPointer<vtkIdList> target_ids = vtkSmartPointer<vtkIdList>::New();
        lines->GetNextCell(target_ids);

        std::vector<std::array<double, 3>> topLinePoints;
        for (vtkIdType i = 0; i < target_ids->GetNumberOfIds(); ++i) {
            double point[3];
            splitedOutline->GetPoint(target_ids->GetId(i), point);
            topLinePoints.push_back({ point[0], point[1], point[2] });
        }

        if (topLinePoints[0][0] > topLinePoints.back()[0]) {
            std::reverse(topLinePoints.begin(), topLinePoints.end());
        }

        auto topY = get_y_from(topLinePoints, outcurveX);

        lines->InitTraversal();
        for (int i = 0; i < sorted_lns[3][0]; ++i) {
            lines->GetNextCell(id_list);
        }

        lines->GetNextCell(target_ids);

        std::vector<std::array<double, 3>> bottomeLinePoints;
        for (vtkIdType i = 0; i < target_ids->GetNumberOfIds(); ++i) {
            double point[3];
            splitedOutline->GetPoint(target_ids->GetId(i), point);
            bottomeLinePoints.push_back({ point[0], point[1], point[2] });
        }

        if (bottomeLinePoints[0][0] > bottomeLinePoints.back()[0]) {
            std::reverse(bottomeLinePoints.begin(), bottomeLinePoints.end());
        }

        auto bottomY = get_y_from(bottomeLinePoints, outcurveX);

        std::vector<double> relativeY(num_samples);
        for (int i = 0; i < num_samples; ++i) {
            relativeY[i] = bottomY[i] + (topY[i] - bottomY[i]) * (relative_R / 100.0);
        }

        std::vector<std::array<double, 3>> relativeCurvePoints(num_samples);
        for (int i = 0; i < num_samples; ++i) {
            relativeCurvePoints[i] = { outcurveX[i], relativeY[i], 0.0 };
        }
        //！！！调用函数
        auto relativeCurve = numpy_to_vtk_polyline(relativeCurvePoints);

        return relativeCurve;
    }

    /***生成旋转面***/
    vtkSmartPointer<vtkPolyData> generateRotationalSurface(vtkSmartPointer<vtkPolyData> curve, double startAngle, double endAngle, int nseg) {
        // Step 1: Preprocess curve to ensure continuous polyline
        auto stripper = vtkSmartPointer<vtkStripper>::New();
        stripper->SetInputData(curve);
        stripper->JoinContiguousSegmentsOn();
        stripper->Update();
        auto processedCurve = stripper->GetOutput();

        // Step 2: Rotate curve to start angle
        auto preRotate = vtkSmartPointer<vtkTransform>::New();
        preRotate->RotateWXYZ(startAngle, 1, 0, 0);
        auto preFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        preFilter->SetTransform(preRotate);
        preFilter->SetInputData(processedCurve);
        preFilter->Update();
        processedCurve = preFilter->GetOutput();

        // Step 3: Create rotational extrusion
        auto extrude = vtkSmartPointer<vtkRotationalExtrusionFilter>::New();
        extrude->SetInputData(processedCurve);
        extrude->SetResolution(nseg);
        extrude->SetAngle(endAngle - startAngle);
        extrude->SetRotationAxis(1, 0, 0);
        extrude->Update();

        // Step 4: Clean duplicate points
        auto cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
        cleaner->SetInputData(extrude->GetOutput());
        cleaner->SetTolerance(1e-6);
        cleaner->Update();
        return cleaner->GetOutput();
    }
    /*使用inputPoly对于inputug进行截面*/
    vtkSmartPointer<vtkPolyData> clipUgWithPolydata(vtkSmartPointer<vtkUnstructuredGrid> inputUg,vtkSmartPointer<vtkPolyData> inputPoly) {
        // Step 1: Convert polygonal surface to implicit function
        auto implicitPoly = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        implicitPoly->SetInput(inputPoly);

        // Step 2: Create bounding box for initial coarse clipping
        double bounds[6];
        inputPoly->GetBounds(bounds);
        auto boxClipper = vtkSmartPointer<vtkBoxClipDataSet>::New();
        boxClipper->SetInputData(inputUg);
        boxClipper->SetBoxClip(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
        boxClipper->Update();
//        vtkSmartPointer<vtkPointData> boxClipperPointData = boxClipper->GetOutput()->GetPointData();
//        qInfo() << "BoxClipper Output PointData Arrays: " << boxClipperPointData->GetNumberOfArrays();
//        for (int i = 0; i < boxClipperPointData->GetNumberOfArrays(); ++i) {
//            qInfo() << "Array " << i << ": " << (boxClipperPointData->GetArrayName(i) ? boxClipperPointData->GetArrayName(i) : "NULL");
//        }
        //正常，此时没有多添加一个arrayname=""，这个""是在cutter产生的

        // Step 3: Perform precise cutting
        auto cutter = vtkSmartPointer<vtkCutter>::New();
        cutter->SetInputData(boxClipper->GetOutput());
        //cutter->SetInputData(inputUg);
        cutter->SetCutFunction(implicitPoly);
        cutter->GenerateCutScalarsOn();
        auto locator = vtkSmartPointer<vtkMergePoints>::New();
        cutter->SetLocator(locator);
        cutter->Update();
        // 获取 cutter 结果
        vtkSmartPointer<vtkPolyData> cutterOutput = cutter->GetOutput();
        vtkSmartPointer<vtkPointData> pointData = cutterOutput->GetPointData();

        // 移除空的 PointDataArray
        //qInfo() << "Before Cleaning: PointData Arrays: " << pointData->GetNumberOfArrays();
        for (int i = pointData->GetNumberOfArrays() - 1; i >= 0; --i) {
            const char* arrayName = pointData->GetArrayName(i);
            if (!arrayName || std::string(arrayName).empty()) {
                //qInfo() << "Removing empty array at index: " << i;
                pointData->RemoveArray(i);
            }
        }
        //qInfo() << "After Cleaning: PointData Arrays: " << pointData->GetNumberOfArrays();
//        // Step 4: Extract surface
//        auto geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
//        geometryFilter->SetInputData(cutterOutput);
//        geometryFilter->Update();


        return cutterOutput;
    }
    vtkSmartPointer<vtkPolyData> clipUgWithPolydataCleaned(vtkSmartPointer<vtkPolyData> inputPoly,vtkSmartPointer<vtkUnstructuredGrid> input,
                                                           vtkSmartPointer<vtkUnstructuredGrid> periodic1,
                                                           vtkSmartPointer<vtkUnstructuredGrid> periodic2,
                                                           vtkSmartPointer<vtkUnstructuredGrid> inlet,
                                                           vtkSmartPointer<vtkUnstructuredGrid> outlet,
                                                           vtkSmartPointer<vtkUnstructuredGrid> shroud,
                                                           vtkSmartPointer<vtkUnstructuredGrid> hub) {
        // Step 1: Convert polygonal surface to implicit function
        auto implicitPoly = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        implicitPoly->SetInput(inputPoly);

        // Step 2: Create bounding box for initial coarse clipping
//        double bounds[6];
//        inputPoly->GetBounds(bounds);
//        auto boxClipper = vtkSmartPointer<vtkBoxClipDataSet>::New();
//        boxClipper->SetInputData(inputUg);
//        boxClipper->SetBoxClip(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
//        boxClipper->Update();
//        vtkSmartPointer<vtkPointData> boxClipperPointData = boxClipper->GetOutput()->GetPointData();
//        qInfo() << "BoxClipper Output PointData Arrays: " << boxClipperPointData->GetNumberOfArrays();
//        for (int i = 0; i < boxClipperPointData->GetNumberOfArrays(); ++i) {
//            qInfo() << "Array " << i << ": " << (boxClipperPointData->GetArrayName(i) ? boxClipperPointData->GetArrayName(i) : "NULL");
//        }
        //正常，此时没有多添加一个arrayname=""，这个""是在cutter产生的

        // Step 3: Perform precise cutting
        auto cutter = vtkSmartPointer<vtkCutter>::New();
        //cutter->SetInputData(boxClipper->GetOutput());
        cutter->SetInputData(input);
        cutter->SetCutFunction(implicitPoly);
        cutter->GenerateCutScalarsOn();
        auto locator = vtkSmartPointer<vtkMergePoints>::New();
        cutter->SetLocator(locator);
        cutter->Update();
        // 获取 cutter 结果
        vtkSmartPointer<vtkPolyData> cutterOutput = cutter->GetOutput();
        vtkSmartPointer<vtkPointData> pointData = cutterOutput->GetPointData();

        // 移除空的 PointDataArray
        //qInfo() << "Before Cleaning: PointData Arrays: " << pointData->GetNumberOfArrays();
        for (int i = pointData->GetNumberOfArrays() - 1; i >= 0; --i) {
            const char* arrayName = pointData->GetArrayName(i);
            if (!arrayName || std::string(arrayName).empty()) {
                //qInfo() << "Removing empty array at index: " << i;
                pointData->RemoveArray(i);
            }
        }
        //qInfo() << "After Cleaning: PointData Arrays: " << pointData->GetNumberOfArrays();
        // Step 4: Extract surface
        auto geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
        geometryFilter->SetInputData(cutterOutput);
        geometryFilter->Update();

        return geometryFilter->GetOutput();
    }
    vtkSmartPointer<vtkUnstructuredGrid> clipWithSixSurfaces(
        vtkSmartPointer<vtkPolyData> input,
        vtkSmartPointer<vtkUnstructuredGrid> p1,
        vtkSmartPointer<vtkUnstructuredGrid> p2,
        vtkSmartPointer<vtkUnstructuredGrid> inlet,
        vtkSmartPointer<vtkUnstructuredGrid> outlet,
        vtkSmartPointer<vtkUnstructuredGrid> shroud,
        vtkSmartPointer<vtkUnstructuredGrid> hub)
    {
        //runtime
        QElapsedTimer timer;
        timer.start();
    //       auto regionImplicit = vtkSmartPointer<vtkImplicitBoolean>::New();
    //           regionImplicit->SetOperationTypeToIntersection();
    //       auto polyFilter = [](vtkSmartPointer<vtkUnstructuredGrid> ug, const QString& name) {
    //               auto geo = vtkSmartPointer<vtkGeometryFilter>::New();
    //               geo->SetInputData(ug);
    //               geo->Update();
    //               auto poly = geo->GetOutput();
    //               qInfo().noquote() << QString("Surface [%1] polys: %2")
    //                                    .arg(name)
    //                                    .arg(poly->GetNumberOfPolys());
    //               return poly;
    //           };

    //           auto P1Poly = polyFilter(periodic1, "Periodic1");
    //           auto P2Poly = polyFilter(periodic2, "Periodic2");
    //           auto inletPoly = polyFilter(inlet, "Inlet");
    //           auto outletPoly = polyFilter(outlet, "Outlet");
    //           auto shroudPoly = polyFilter(shroud, "Shroud");
    //           auto hubPoly = polyFilter(hub, "Hub");

    //           // 构造隐式距离函数
    //           auto addImplicit = [&](vtkPolyData* pd) {
    //               auto impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    //               impl->SetInput(pd);
    //               regionImplicit->AddFunction(impl);
    //           };
    //           addImplicit(inletPoly);
    //           addImplicit(outletPoly);
    //           addImplicit(P1Poly);
    //           addImplicit(P2Poly);
    //           addImplicit(shroudPoly);
    //           addImplicit(hubPoly);

    //           auto clipper = vtkSmartPointer<vtkClipDataSet>::New();
    //           clipper->SetInputData(input);
    //           clipper->SetClipFunction(regionImplicit);
    //           clipper->InsideOutOn();
    //           clipper->Update();

    //           qint64 elapsed = timer.elapsed();
    //           qInfo() << "clipWithSixSurfaces done in" << elapsed << "ms";

    //           return clipper->GetOutput();
        // 1. 创建隐式函数对象
        auto regionImplicit = vtkSmartPointer<vtkImplicitBoolean>::New();
        regionImplicit->SetOperationTypeToIntersection();  // 所有面都为必须满足

        //全部ug转为polydata
        auto p1Poly = vtkSmartPointer<vtkGeometryFilter>::New();
        p1Poly->SetInputData(p1);
        p1Poly->Update();
        auto p2Poly = vtkSmartPointer<vtkGeometryFilter>::New();
        p2Poly->SetInputData(p2);
        p2Poly->Update();
        auto inletPoly = vtkSmartPointer<vtkGeometryFilter>::New();
        inletPoly->SetInputData(inlet);
        inletPoly->Update();
        auto outletPoly = vtkSmartPointer<vtkGeometryFilter>::New();
        outletPoly->SetInputData(outlet);
        outletPoly->Update();
        auto shroudPoly = vtkSmartPointer<vtkGeometryFilter>::New();
        shroudPoly->SetInputData(shroud);
        shroudPoly->Update();
        auto hubPoly = vtkSmartPointer<vtkGeometryFilter>::New();
        hubPoly->SetInputData(hub);
        hubPoly->Update();
        // === inlet: 保留面之后（正向） ===
        auto p1Impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        p1Impl->SetInput(p1Poly->GetOutput());
        regionImplicit->AddFunction(p1Impl);

        auto p2Impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        p2Impl->SetInput(p2Poly->GetOutput());
        regionImplicit->AddFunction(p2Impl);

        auto inletImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        inletImpl->SetInput(inletPoly->GetOutput());
        regionImplicit->AddFunction(inletImpl);

        // === outlet: 保留面之前（反向） ===
        auto outletImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        outletImpl->SetInput(outletPoly->GetOutput());
        regionImplicit->AddFunction(outletImpl);

        auto shroudImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        shroudImpl->SetInput(shroudPoly->GetOutput());
        regionImplicit->AddFunction(shroudImpl);

        auto hubImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
        hubImpl->SetInput(hubPoly->GetOutput());
        regionImplicit->AddFunction(hubImpl);

        // 2. 执行 clip 操作
        auto clipper = vtkSmartPointer<vtkClipDataSet>::New();
        clipper->SetInputData(input);
        //clipper->SetInputData(boxClipper->GetOutput());
        clipper->SetClipFunction(regionImplicit);
        clipper->InsideOutOn();  // 保留“封闭盒”内部区域
        clipper->Update();


        // 3. 拷贝结果（可避免依赖 clipper 管线）
    //    auto result = vtkSmartPointer<vtkUnstructuredGrid>::New();
    //    result->DeepCopy(clipper->GetOutput());
    //    qInfo()<<"function return result";
    //    return result;
                   qint64 elapsed = timer.elapsed();
                   qInfo() << "clipWithSixSurfaces done in" << elapsed << "ms";
        return clipper->GetOutput();
    }
    /**S1面提取的主要入口***/

    vtkSmartPointer<vtkUnstructuredGrid> s1Extract(vtkSmartPointer<vtkUnstructuredGrid> p1SurfaceName,vtkSmartPointer<vtkUnstructuredGrid> p2SurfaceName,vtkSmartPointer<vtkUnstructuredGrid> inlet,vtkSmartPointer<vtkUnstructuredGrid>outlet,vtkSmartPointer<vtkUnstructuredGrid>shroud,vtkSmartPointer<vtkUnstructuredGrid>hub,vtkSmartPointer<vtkUnstructuredGrid> ug,double relativeR = 50.0) {
        //// Read the unstructured grid
        //auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        //auto ugreader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        //ugreader->SetFileName(blockFileName.c_str());
        //ugreader->Update();
        //ug = ugreader->GetOutput();
          // 转换为 PolyData
        vtkSmartPointer<vtkGeometryFilter> geometryFilter =
            vtkSmartPointer<vtkGeometryFilter>::New();
        geometryFilter->SetInputData(p1SurfaceName);
        geometryFilter->Update();

        vtkSmartPointer<vtkPolyData> p1Name = geometryFilter->GetOutput();
        std::vector<vtkSmartPointer<vtkPolyData>> periodicFace;//只保留了周期面1
        periodicFace.push_back(p1Name);
        // Perform meridional projection子午面投影
        auto meridionalSurface = meridionalProjection(periodicFace);

        // Generate radial curve at specified relative height
        auto s1Curve = generate_radial_curve(meridionalSurface, relativeR, 20);

        // Calculate theta bounds and generate rotational surface
        int nseg = 10;
        auto angleBounds = calculateThetaBounds(ug);
        double startAngle = angleBounds.first;
        double endAngle = angleBounds.second;
        auto rotationalSurface = generateRotationalSurface(s1Curve, startAngle, endAngle, nseg);


        // Probe the unstructured grid with the rotational surface
        //auto result = probeUgWithPolydata(ug, rotationalSurface);
        //auto result = clipUgWithPolydata(ug, rotationalSurface);
        auto result=clipUgWithPolydata(ug, rotationalSurface);//旋转面切割出来的结果
        auto target=clipWithSixSurfaces(result,p1SurfaceName,p2SurfaceName,inlet,outlet,shroud,hub);
        return target;
    }
///////////////////开始S2部分
    vtkSmartPointer<vtkPolyData> generate_s2_surface(
        vtkSmartPointer<vtkPolyData> period_surface,// 原始周期面数据
        vtkSmartPointer<vtkUnstructuredGrid> volume_data,// // 三维流场数据
        double start_angle=0, // 旋转起始角度
        double end_angle=360,// 旋转结束角度
        int num_steps = 10// 旋转结束角度
    ) {
        // Step 1: Extract X and R ranges from period_surface
        //qInfo()<<"开始Step1";
        vtkSmartPointer<vtkPoints> points = period_surface->GetPoints();
        int num_points = points->GetNumberOfPoints();
       // qInfo()<<"num_points:"<<num_points;
        double x_min = VTK_DOUBLE_MAX, x_max = VTK_DOUBLE_MIN;
        double r_min = VTK_DOUBLE_MAX, r_max = VTK_DOUBLE_MIN;
        for (int i = 0; i < num_points; ++i) {
            double p[3];
            points->GetPoint(i, p);
            double x = p[0];
            double r = std::sqrt(p[1] * p[1] + p[2] * p[2]);
            if (x < x_min) x_min = x;
            if (x > x_max) x_max = x;
            if (r < r_min) r_min = r;
            if (r > r_max) r_max = r;
        }

        // Step 2: Initialize accumulation arrays初始化数据存储
        // 获取 period_surface 的所有 point data 数组
        vtkPointData* pointData = period_surface->GetPointData();
        int numArrays = pointData->GetNumberOfArrays();
        //qInfo()<<"Step1,num_points:"<<num_points;
        // 存储所有数组的累加值和有效计数
        std::vector<vtkSmartPointer<vtkDoubleArray>> sumArrays(numArrays);
        std::vector<std::vector<double>> valueSums(numArrays, std::vector<double>(num_points, 0.0));
        std::vector<std::vector<int>> validCounts(numArrays, std::vector<int>(num_points, 0));
        for (int i = 0; i < numArrays; ++i) {
            vtkDataArray* array = pointData->GetArray(i);
            if (array) {
                sumArrays[i] = vtkSmartPointer<vtkDoubleArray>::New();
                sumArrays[i]->SetName(array->GetName());
                sumArrays[i]->SetNumberOfComponents(array->GetNumberOfComponents());
                sumArrays[i]->SetNumberOfTuples(num_points);
            }
        }

        // Step 3: Create transform and filters设置旋转变换和插值
        vtkSmartPointer<vtkTransform> angle_transform = vtkSmartPointer<vtkTransform>::New();
        vtkSmartPointer<vtkTransformFilter> transform_filter = vtkSmartPointer<vtkTransformFilter>::New();
        transform_filter->SetInputData(period_surface);
        transform_filter->SetTransform(angle_transform);

        vtkSmartPointer<vtkProbeFilter> probe_filter = vtkSmartPointer<vtkProbeFilter>::New();
        probe_filter->SetSourceData(volume_data);

        // Step 4: Rotate and probe旋转 period_surface 并进行插值
        double angle_step = (end_angle - start_angle) / num_steps;
        for (int i = 0; i <= num_steps; ++i) {
            double current_angle = start_angle + i * angle_step;
            angle_transform->Identity();
            angle_transform->RotateX(current_angle);
            transform_filter->Update();

            probe_filter->SetInputConnection(transform_filter->GetOutputPort());
            probe_filter->Update();
            vtkPolyData* probed_data = probe_filter->GetPolyDataOutput();

            vtkPointData* probedPointData = probed_data->GetPointData();
            vtkIntArray* mask = vtkIntArray::SafeDownCast(probedPointData->GetArray("vtkValidPointMask"));
            for (int arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx) {
                vtkDataArray* probedArray = probedPointData->GetArray(sumArrays[arrayIdx]->GetName());
                if (!probedArray) continue;

                for (int j = 0; j < num_points; ++j) {
                    if (!mask || mask->GetValue(j)) {  // 仅处理有效点
                        for (int comp = 0; comp < probedArray->GetNumberOfComponents(); ++comp) {
                            valueSums[arrayIdx][j] += probedArray->GetComponent(j, comp);
                        }
                        validCounts[arrayIdx][j]++;
                    }
                }
            }
        }
        //// Step 5: Compute mean pressure计算所有压力的平均值
        for (int arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx) {
            vtkSmartPointer<vtkDoubleArray> avgArray = sumArrays[arrayIdx];
            for (int j = 0; j < num_points; ++j) {
                for (int comp = 0; comp < avgArray->GetNumberOfComponents(); ++comp) {
                    double meanValue = (validCounts[arrayIdx][j] > 0)
                        ? valueSums[arrayIdx][j] / validCounts[arrayIdx][j]
                        : 0.0;
                        avgArray->SetComponent(j, comp, meanValue);
                }
            }
            if (avgArray->GetNumberOfTuples() > 0) {
                period_surface->GetPointData()->AddArray(avgArray);
            }
            else {
                std::cerr << "Skipping empty array: " << avgArray->GetName() << std::endl;
            }
            //period_surface->GetPointData()->AddArray(avgArray);
        }
        // Step 6: Generate meridional projection生成子午面
        std::vector<vtkSmartPointer<vtkPolyData> > inputs = { period_surface };
        vtkSmartPointer<vtkPolyData> meridional_grid = meridionalProjection(inputs);
        // Step 7: Set normals
        vtkSmartPointer<vtkDoubleArray> transformed_normals =
            vtkDoubleArray::SafeDownCast(meridional_grid->GetPointData()->GetNormals());
        if (!transformed_normals) {
            transformed_normals = vtkSmartPointer<vtkDoubleArray>::New();
            transformed_normals->SetNumberOfComponents(3);
            transformed_normals->SetNumberOfTuples(meridional_grid->GetNumberOfPoints());
        }
        for (int i = 0; i < meridional_grid->GetNumberOfPoints(); ++i) {
            transformed_normals->SetTuple3(i, 0, 1, 1);
        }
        //meridional_grid->GetPointData()->SetNormals(transformed_normals);添加了一个空的pointdata

        return meridional_grid;
    }
}

TecplotWidget::TecplotWidget(QWidget *parent)
    :QVTKOpenGLNativeWidget(parent)
{
    // 初始化渲染器和渲染窗口等所有功能共享部分
    this->m_multiBlock=vtkSmartPointer<vtkMultiBlockDataSet>::New();
    this->m_unstructuredGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
    this->m_pointData=vtkSmartPointer<vtkPointData>::New();
    this->m_varNum = 0;

    this->m_renderer = vtkSmartPointer<vtkRenderer>::New();
    this->m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    this->m_renderWindow->AddRenderer(m_renderer);
    this->setRenderWindow(m_renderWindow);
    QColor lightBlue(82, 87, 110);
    this->m_renderer->SetBackground(lightBlue.redF(), lightBlue.greenF(), lightBlue.blueF());
    //获取交互器
    this->m_qvtkInteractor=this->interactor();

    //坐标
    m_axes = vtkSmartPointer<vtkAxesActor>::New();
    m_orientationMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();

    m_orientationMarker->SetOrientationMarker(m_axes);
    m_orientationMarker->SetInteractor(m_renderWindow->GetInteractor());
    // m_orientationMarker->SetViewport(0.0, 0.0, 0.3, 0.3);
    m_orientationMarker->SetViewport(0.0, 0.0, 0.12, 0.4);
    m_orientationMarker->SetEnabled(1);
    m_orientationMarker->InteractiveOff();

    m_renderer->SetBackground2(1.0, 1.0, 1.0); // 设置页面底部颜色值
    m_renderer->SetBackground(0.529, 0.8078, 0.92157); // 设置页面顶部颜色值
    m_renderer->SetGradientBackground(true); // 开启渐变色背景设置
}
TecplotWidget::~TecplotWidget()
{
}
/**设置需要读入的文件的路径，对block0进行绘制？？**/
void TecplotWidget::SetFileName(QString fileName)
{
    this->m_multiBlock = this->m_reader.ReadTecplotData(fileName.toStdString());
    //qInfo()<<"after read";
    if (!this->m_multiBlock) {
            QMessageBox::warning(this, "Warning", "文件读取失败");
            return;
    }
    this->m_blockNum = this ->m_multiBlock->GetNumberOfBlocks();
    //qInfo()<<"blockNum:"<<this->m_blockNum;
    //将每个block作为一个actor渲染，并且以block的name来命名
    std::string blockName = "";
    for(int i = 0;i < this->m_blockNum;i++)
    {
        const char* Name = this->m_multiBlock->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
        blockName = Name;

        auto tmpMapper=vtkSmartPointer<vtkDataSetMapper>::New();
        auto tmpActor=vtkSmartPointer<vtkActor>::New();
        m_actorsList[blockName]=tmpActor;
        this->m_actorsStatus[blockName]=true;
        //qInfo()<<vtkUnstructuredGrid::SafeDownCast(this->m_multiBlock->GetBlock(i))->GetPointData()->GetNumberOfArrays();
        //qInfo()<<vtkUnstructuredGrid::SafeDownCast(this->m_multiBlock->GetBlock(i))->GetNumberOfPoints();
        tmpMapper->SetInputData(vtkUnstructuredGrid::SafeDownCast(this->m_multiBlock->GetBlock(i)));
        tmpActor->SetMapper(tmpMapper);
        this->m_renderer->AddActor(tmpActor);
        blockName.clear();
    }
    this->m_renderWindow->Render();
}
int TecplotWidget::GetNumberOfBlock()
{
    if(this->m_multiBlock->GetNumberOfBlocks()==0)
    {
        QMessageBox::warning(this, "Warning", "请先打开文件");
        return 0;
    }
    return this->m_multiBlock->GetNumberOfBlocks();
}
QStringList TecplotWidget::GetActorList()
{
    QStringList actorNameList;
    for (const auto& pair : m_actorsList)
    {
        actorNameList << QString::fromStdString(pair.first);
    }
    return actorNameList;
}
bool TecplotWidget::ActorVisibilityOn(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(actorName.contains("Slice"))
    {
        // 检查widget是否之前是启用的
               if(this->m_sliceWidgetVisibilityStatus[name])
               {
                   this->m_sliceWigetList[name]->EnabledOn();
               }
    }
    if(this->m_barsStatus.count(name)!=0){
        if(this->m_barsStatus[name])
        {
            this->m_barsList[name]->VisibilityOn();
            if(this->m_colorLineStatus.count(name)!=0&&this->m_colorLineStatus[name]){
                this->m_colorLineList[name]->VisibilityOn();
            }
        }
    }
    this->m_actorsStatus[name] = true;
    vtkActor* objActor = this->m_actorsList[name];
    objActor->VisibilityOn();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::ActorVisibilityOff(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(actorName.contains("Slice"))
    {
        this->m_sliceWigetList[name]->EnabledOff();
    }
    if(this->m_barsList.count(name)!=0)
    {
        this->m_barsList[name]->VisibilityOff();
        //this->m_barsStatus[name] = false;
        if(this->m_colorLineStatus.count(name)!=0){
            this->m_colorLineList[name]->VisibilityOff();
        }
    }
    this->m_actorsStatus[name] = false;
    vtkActor* objActor = this->m_actorsList[name];
    objActor->VisibilityOff();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::RemoveActor(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(this->m_barsList.count(name)!=0)
    {
        auto barActor = this->m_barsList[name];
        this->m_renderer->RemoveActor(barActor);
        this->m_barsList.erase(name);
        this->m_barsStatus.erase(name);
        this->m_lutsList.erase(name);
        if(this->m_colorLineList.count(name)!=0){
            auto colorLine=this->m_colorLineList[name];
            this->m_renderer->RemoveActor(colorLine);
            this->m_numOfColorsList.erase(name);
            this->m_colorMapPropertysList.erase(name);
            this->m_colorLineStatus.erase(name);
            this->m_colorLineList.erase(name);
            this->m_ColorLineContourFilterList.erase(name);
            auto it=std::find(this->m_activeBars.begin(),this->m_activeBars.end(),name);
            if(it!=this->m_activeBars.end()) this->m_activeBars.erase(it);
        }
    }
    if(this->m_lutsList.count(name)!=0)
    {
        this->m_lutsList.erase(name);
    }
    if(actorName.contains("Slice"))
    {
        this->m_cutterList.erase(name);
        this->m_sliceWigetList.erase(name);
        this->m_sliceWidgetVisibilityStatus.erase(name);
        this->m_slicePlaneRepList.erase(name);
        this->m_sliceWidgetNum--;
    }else if(actorName.contains("StreamTracer"))
    {
        this->m_streamTraceNum--;
        this->m_streamTraceList.erase(name);
        this->m_streamTraceMaskPointsList.erase(name);
    }else if(actorName.contains("Glyph"))
    {
        vtkActor* objActor = this->m_actorsList[name];
        this->m_renderer->RemoveActor(objActor);
        this->m_actorsList.erase(name);
        this->m_actorsStatus.erase(name);
        this->m_glyphNum--;
        Glyph* tmp = this->m_glyphsList[name];
        delete tmp;
        this->m_glyphsList.erase(name);
        this->m_renderWindow->Render();
        return true;
    }else if(actorName.contains("Contour"))
    {
        vtkActor* objActor = this->m_actorsList[name];
        this->m_renderer->RemoveActor(objActor);
        this->m_actorsList.erase(name);
        this->m_actorsStatus.erase(name);
        this->m_contourNum--;
        Contour* tmp = this->m_contoursList[name];
        delete tmp;
        this->m_contoursList.erase(name);
        this->m_renderWindow->Render();
        return true;
    }

    vtkActor* objActor = this->m_actorsList[name];
    this->m_renderer->RemoveActor(objActor);
    this->m_actorsList.erase(name);
    this->m_actorsStatus.erase(name);
    this->m_renderWindow->Render();
    return true;
}
int TecplotWidget::GetNumberOfProperty(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    int num = dataSet->GetPointData()->GetNumberOfArrays();
    for(int i = 0;i < num ;i++)
    {
        cout <<endl <<dataSet->GetPointData()->GetArrayName(i);
    }
    return num;
}
QStringList TecplotWidget::GetPropertyList(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());

    QStringList propertyList;
    int num = dataSet->GetPointData()->GetNumberOfArrays();
    for(int i = 0;i < num;i++)
    {
        propertyList<<dataSet->GetPointData()->GetArrayName(i);
    }
    return propertyList;
}
QString TecplotWidget::GetPropertyName(QString actorName,int id)
{//??没测试return啊
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    auto propertyName = dataSet->GetPointData()->GetArrayName(id);
    //QString name(propertyName);
    return QString::fromUtf8(propertyName);
}
std::vector<double> TecplotWidget::GetPropertyBounds(QString actorName, QString propertyName){
    std::vector<double> bounds;
    if (m_actorsList.find(actorName.toStdString()) == m_actorsList.end()) {
            //std::cerr << "Error: Actor " << actorName.toStdString() << " not found!" << std::endl;
            return bounds;
        }
    // 获取 Actor 对应的 vtkDataSet
        vtkActor* actor = m_actorsList[actorName.toStdString()];
        vtkDataSetMapper* mapper = vtkDataSetMapper::SafeDownCast(actor->GetMapper());
        vtkDataSet* dataSet = mapper->GetInput();
        if (!dataSet) {
            //std::cerr << "Error: No input data for " << actorName.toStdString() << std::endl;
            return bounds;
        }

        // 获取 PointData 中的属性数组
        vtkPointData* pointData = dataSet->GetPointData();
        if (!pointData) {
            //std::cerr << "Error: No point data for " << actorName.toStdString() << std::endl;
            return bounds;
        }

        vtkDataArray* array = pointData->GetArray(propertyName.toStdString().c_str());
        if (!array) {
            //std::cerr << "Error: Property " << propertyName.toStdString() << " not found in "<< actorName.toStdString() << std::endl;
            return bounds;
        }
        int numComponents = array->GetNumberOfComponents(); // 获取分量数
        if (numComponents == 1) {
                // 标量情况，返回大小为 2 的向量
                bounds.resize(2);
                array->GetRange(bounds.data());
                std::cout << "Scalar bounds for " << propertyName.toStdString() << " in "
                          << actorName.toStdString() << ": [" << bounds[0] << ", " << bounds[1] << "]" << std::endl;
            } else {
                // 向量情况，返回大小为 numComponents * 2 的向量（每个分量的 min 和 max）
                bounds.resize(numComponents * 2);
                for (int comp = 0; comp < numComponents; comp++) {
                    array->GetRange(&bounds[2 * comp], comp); // 获取第 comp 分量的范围
                }
                std::cout << "Vector bounds for " << propertyName.toStdString() << " in "
                          << actorName.toStdString() << " (" << numComponents << " components): ";
                for (int i = 0; i < numComponents; i++) {
                    std::cout << "[" << bounds[2 * i] << ", " << bounds[2 * i + 1] << "]";
                    if (i < numComponents - 1) std::cout << ", ";
                }
                std::cout << std::endl;
            }

            return bounds;
}

void TecplotWidget::SetBackgroundColor(QColor color)
{
    int r=color.red();
    int g=color.green();
    int b=color.blue();
    double normalizedR = r / 255.0;
    double normalizedG = g / 255.0;
    double normalizedB = b / 255.0;

    m_renderer->SetBackground(normalizedR, normalizedG, normalizedB);
    //？？是否每次都要设置后都要重新渲染
    m_renderWindow->Render();
}
QColor TecplotWidget::GetBackgroundColor()
{
    double* rgb = m_renderer->GetBackground();
    int r = static_cast<int>(rgb[0] * 255);
    int g = static_cast<int>(rgb[1] * 255);
    int b = static_cast<int>(rgb[2] * 255);
    return QColor(r, g, b);
}
void TecplotWidget::SetSolidColor(QString actorName,QColor color)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    if(this->m_barsList.count(actorName.toStdString())!=0)
    {//如果之前执行过颜色映射，需要关闭颜色映射、设置bar不可见
        objActor->GetMapper()->ScalarVisibilityOff();
        this->m_barsList[actorName.toStdString()]->VisibilityOff();
        this->m_barsStatus[actorName.toStdString()]= false;
    }
    int r=color.red();
    int g=color.green();
    int b=color.blue();
    double normalizedR = r / 255.0;
    double normalizedG = g / 255.0;
    double normalizedB = b / 255.0;
    objActor->GetProperty()->SetColor(normalizedR,normalizedG,normalizedB);
    m_renderWindow->Render();
}
void TecplotWidget::SetSolidOpacity(QString actorName, double opacity)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    if (this->m_barsList.count(actorName.toStdString()) != 0)
//    {
//        // 关闭颜色映射和颜色条
//        objActor->GetMapper()->ScalarVisibilityOff();
//        this->m_barsList[actorName.toStdString()]->VisibilityOff();
//        this->m_barsStatus[actorName.toStdString()] = false;
//    }
    // 设置透明度（范围 0.0 ~ 1.0）
    objActor->GetMapper()->ScalarVisibilityOff();
    objActor->GetProperty()->SetOpacity(opacity);
    m_renderWindow->Render();
}
QColor TecplotWidget::GetSolidColor(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    double* rgb=objActor->GetProperty()->GetColor();
    return QColor::fromRgbF(rgb[0],rgb[1],rgb[2]);
}
bool TecplotWidget::SetColorMapOn(QString actorName,QString propertyName)
{
    std::string name=propertyName.toStdString();
    int num=15-name.length();
    std::string pre="";
    for(int i=0;i<num;i++){
        pre=pre+"  ";
    }
    std::string end=name+pre;
    std::string objName = actorName.toStdString();
    std::string objVar = propertyName.toStdString();
    vtkActor* objActor = this->m_actorsList[objName];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    //如果采用默认参数，则必须之前打开过颜色映射
    if(this->m_barsList.count(objName)==0)
    {
        if(propertyName=="") return false;
        //该actor第一次打开颜色映射，需要新建lut
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        auto barActor = vtkSmartPointer<vtkScalarBarActor>::New();
        this->m_lutsList[objName] = lut;
        this->m_barsList[objName] = barActor;
        this->m_renderer->AddActor2D(barActor);
        this->m_barsStatus[objName] = true;
        m_numOfColorsList[objName]=10;
        m_activeBars.push_back(objName);
        barActor->SetLabelFormat("%g");  // 使用%g自动选择最合适的格式
        barActor->SetTitle(end.c_str());
        barActor->SetNumberOfLabels(10);
        barActor->SetVerticalTitleSeparation(9);//让颜色条的标题与色阶有一定的距离

    }
    m_colorMapPropertysList[objName]=objVar;
    auto lut = m_lutsList[objName];
    auto barActor = m_barsList[objName];

    if(propertyName=="")
    {
        mapper->ScalarVisibilityOn();
        barActor->VisibilityOn();
        this->m_barsStatus[objName] = true;
        auto it = std::find(m_activeBars.begin(), m_activeBars.end(), objName);
        if (it == m_activeBars.end()) {
            m_activeBars.push_back(objName);
        }
        UpdateAllScalarBarPositions();
        this->m_renderWindow->Render();
        return true;
    }
    // 设置 Active Scalars
    //m_numOfColorsList[objName]=10;
    int selectedId = dataSet->GetPointData()->SetActiveScalars(objVar.c_str());
    vtkDataArray* scalar = dataSet->GetPointData()->GetArray(selectedId);
    lut->SetTableRange(scalar->GetRange());
    lut->SetNumberOfColors(256);
    lut->SetHueRange(0.666, 0.0);
    lut->Build();
    barActor->SetLookupTable(lut);
    barActor->SetTitle(end.c_str());
    mapper->SetScalarRange(scalar->GetRange());
    mapper->SetLookupTable(lut);
    mapper->ScalarVisibilityOn();
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SelectColorArray(propertyName.toStdString().c_str());  // 指定颜色映射属性
    barActor->VisibilityOn();
//    mapper->UseLookupTableScalarRangeOn();//这句话会影响，bar色阶显示的是range还是lut的range/
    // 强制更新颜色条位置
    // 添加到激活列表并更新布局
    this->m_barsStatus[objName] = true;
    auto it = std::find(m_activeBars.begin(), m_activeBars.end(), objName);
    if (it == m_activeBars.end()) {
        m_activeBars.push_back(objName);
    }
    UpdateAllScalarBarPositions();
    m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetColorMapOff(QString actorName)
{
    std::string objStdName = actorName.toStdString();
    if(this->m_actorsList.count(objStdName)==0)
        return false;
    vtkActor* objActor = this->m_actorsList[objStdName];

    if(this->m_barsList.count(objStdName)!=0)
    {//如果之前执行过颜色映射，需要关闭颜色映射、设置bar不可见
        objActor->GetMapper()->ScalarVisibilityOff();
        this->m_barsList[objStdName]->VisibilityOff();
        this->m_barsStatus[objStdName] = false;
    }
    m_renderWindow->Render();
    // 从激活列表移除
    auto it = std::find(m_activeBars.begin(), m_activeBars.end(), objStdName);
    if(it != m_activeBars.end()) {
        m_activeBars.erase(it);
        UpdateAllScalarBarPositions();  // 更新剩余颜色条
    }
    return true;
}
bool TecplotWidget::SetNumberOfColor(QString actorName,int colorNum){
    std::string objStdName = actorName.toStdString();
        if(this->m_actorsList.count(objStdName)==0) return false;
        if(this->m_barsList.count(objStdName)!=0)
        {
            this->m_barsList[objStdName]->SetNumberOfLabels(colorNum);
            m_numOfColorsList[objStdName]=colorNum;
            this->m_barsList[objStdName]->Modified();
        }
        m_renderWindow->Render();
        return true;
}
bool TecplotWidget::TecplotWidget::SetColorMapBounds(QString actorName, double low, double high) {
    std::string objStdName = actorName.toStdString();
        if (this->m_actorsList.count(objStdName) == 0) return false;

        if (this->m_barsList.count(objStdName) != 0) { // 颜色渐变的级数
            auto lut = this->m_lutsList[objStdName];

            for (int i = 0; i < 256; i++) {
                    double val = lut->GetRange()[0] + (lut->GetRange()[1] - lut->GetRange()[0]) * i / 255.0;
                    if (val < low) {
                        lut->SetTableValue(i, 0.0, 0.0, 1.0);  // 纯蓝
                    } else if (val > high) {
                        lut->SetTableValue(i, 1.0, 0.0, 0.0);  // 纯红
                    } else {
                        double ratio = (val - low) / (high - low);
                        if (ratio < 0.5) {
                            lut->SetTableValue(i, 0.0, ratio * 2.0, 1.0 - ratio * 2.0);
                        } else {
                            lut->SetTableValue(i, (ratio - 0.5) * 2.0, 1.0 - (ratio - 0.5) * 2.0, 0.0);
                        }
                    }
                }

            lut->SetTableRange(low, high);  // **强制颜色映射适应新范围**
            lut->Build();
            lut->Modified();
        }

        m_renderWindow->Render();
        return true;
}
void TecplotWidget::HideScalarBars(const QStringList& actorNames)
{
    for (const QString& name : actorNames) {
        std::string actorName = name.toStdString();
        if (this->m_barsList.count(actorName)) {
            vtkScalarBarActor* barActor = this->m_barsList[actorName];
            barActor->VisibilityOff();  // 仅关闭色阶条的可见性
            this->m_barsStatus[actorName] = false;

            // 从 m_activeBars 中移除
            auto it = std::find(m_activeBars.begin(), m_activeBars.end(), actorName);
            if (it != m_activeBars.end()) {
                    m_activeBars.erase(it);
            }
        }
    }
    UpdateAllScalarBarPositions();  // 重新布局剩余的颜色条
    this->m_renderWindow->Render();  // 重新渲染更新可见性
}
bool TecplotWidget::SetColorLineOn(QString actorName){
    std::string objStdName = actorName.toStdString();
    if(this->m_barsList.count(objStdName)==0){
        return false;
    }
    vtkActor* objActor = this->m_actorsList[objStdName];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    if(this->m_colorLineList.count(objStdName)==0)
    {//第一次打开等值线，需要新建contourfilter和actor、搭建管线
        vtkSmartPointer<vtkContourFilter> contourFilter = vtkSmartPointer<vtkContourFilter>::New();
        this->m_ColorLineContourFilterList[objStdName]=contourFilter;
        contourFilter->SetInputData(dataSet);
        contourFilter->Update();
        vtkSmartPointer<vtkPolyDataMapper> contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        contourMapper->SetInputConnection(contourFilter->GetOutputPort());
        vtkSmartPointer<vtkActor> contourActor = vtkSmartPointer<vtkActor>::New();
        contourActor->SetMapper(contourMapper);
        contourActor->GetProperty()->SetColor(0,0,0);
        m_colorLineList[objStdName]=contourActor;
        m_renderer->AddActor(contourActor);
    }
    int scalarId=dataSet->GetPointData()->SetActiveScalars(m_colorMapPropertysList[objStdName].c_str());
    vtkDataArray* activeScalars = dataSet->GetPointData()->GetArray(scalarId);
    if (!activeScalars) {
//        qInfo()<< m_colorActorPropertysList[objStdName].c_str()<<"不存在或设置失败";
        return false;
    }
    auto contourFilter = this->m_ColorLineContourFilterList[objStdName];
    auto lut = this->m_lutsList[objStdName];
    contourFilter->GenerateValues(m_numOfColorsList[objStdName], lut->GetRange()[0],lut->GetRange()[1]);
//    qInfo()<<"line"<<m_lutsNum[objStdName]<<" "<<lut->GetRange()[0]<<" "<<lut->GetRange()[1];
    contourFilter->Update();
    contourFilter->Modified();
    this->m_colorLineStatus[objStdName]=true;
    m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetColorLineOff(QString actorName){
    std::string objStdName = actorName.toStdString();
    if(this->m_colorLineList.count(objStdName)==0)
    {
        return false;
    }
    this->m_colorLineStatus[objStdName]=false;
    this->m_colorLineList[objStdName]->VisibilityOff();
    this->m_renderWindow->Render();
    return true;
}
// 新增私有方法
/**** 有问题的动态调整，标题大小改变不了***/
 void TecplotWidget::UpdateAllScalarBarPositions()
{
    const int totalBars = static_cast<int>(m_activeBars.size());
    if(totalBars == 0) return;
    // 固定宽度和间距（不再动态调整）
    const float actualBarWidth = MIN_BAR_WIDTH;  // 固定宽度
    const float actualSpacing = HORIZONTAL_SPACING;  // 固定间距

    // 起始位置计算（右对齐）
    float startX = 1.0f - (actualBarWidth * totalBars
                              + actualSpacing * (totalBars - 1))
                              - 0.02f; // 右侧留白2%

    // 统一垂直位置（Y坐标固定）
    const float barHeight = 0.8f;     // 固定高度25%
    const float verticalPos = 0.1f;   // 底部留出25%空间

    // 按激活顺序排列（最新在左侧）
    float currentX = startX;
    for(auto it = m_activeBars.rbegin(); it != m_activeBars.rend(); ++it){
        if(auto bar = m_barsList[*it]){
            bar->SetPosition(currentX, verticalPos);
            bar->SetWidth(actualBarWidth);
            bar->SetHeight(barHeight);
            currentX += actualBarWidth + actualSpacing;
        }
    }
    m_renderWindow->Render();
}
//void TecplotWidget::UpdateAllScalarBarPositions()
//{
//    const int totalBars = static_cast<int>(m_activeBars.size());
//    if (totalBars == 0) return;

//    // 固定宽度和间距（不再动态调整）
//    const float actualBarWidth = MIN_BAR_WIDTH;  // 固定宽度
//    const float actualSpacing = HORIZONTAL_SPACING;  // 固定间距

//    // 起始位置计算（右对齐）
//    float startX = 1.0f - (actualBarWidth * totalBars
//                           + actualSpacing * (totalBars - 1))
//                           - 0.02f; // 右侧留白2%

//    // 统一垂直位置（Y坐标固定）
//    const float barHeight = 0.8f;     // 固定高度
//    const float verticalPos = 0.1f;   // 底部留出一定空间

//    // 按激活顺序排列（最新在右侧）
//    float currentX = startX;
//    for (const auto& barName : m_activeBars) {
//        if (auto bar = m_barsList[barName]) {
//            // 设置尺寸和位置
//            bar->SetPosition(currentX, verticalPos);
//            bar->SetWidth(actualBarWidth);
//            bar->SetHeight(barHeight);

//            // 固定字体大小（不再动态调整）
//            bar->GetTitleTextProperty()->SetFontSize(12);  // 标题字体大小
//            bar->GetLabelTextProperty()->SetFontSize(10);  // 标签字体大小

//            // 确保文字不超出边界
//            bar->SetAnnotationTextScaling(0);
//            //bar->SetTitleRatio(0.1); // 标题占总高度的30%

//            currentX += actualBarWidth + actualSpacing;
//        }
//    }

//    m_renderWindow->Render();
//}
QString TecplotWidget::AddSliceWidget(QString derivedActorName)
{
    this->m_sliceWidgetNum++;
    std::string name = "Slice"+std::to_string(m_sliceWidgetNum);
    vtkActor* objActor = this->m_actorsList[derivedActorName.toStdString()];
    this->m_sliceWidgetVisibilityStatus[name]=true;//为了控制显示，新增加的25.04.29
    vtkDataSet* data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());

    vtkSmartPointer<vtkImplicitPlaneWidget2> cutPlaneWidget = vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
    this->m_sliceWigetList[name] = cutPlaneWidget;
    vtkSmartPointer<vtkImplicitPlaneRepresentation> cutPlaneRep = vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();
    this->m_slicePlaneRepList[name] = cutPlaneRep;
    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    this->m_cutterList[name] = cutter;
    cutter->SetInputData(data);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> cutActor = vtkSmartPointer<vtkActor>::New();
    cutActor->SetMapper(mapper);
    this->m_actorsList[name] = cutActor;
    this->m_actorsStatus[name] = true;
    this->m_renderer->AddActor(cutActor);

//    // 关键修改：设置包围盒并保留空余
//      double bounds[6];
//      data->GetBounds(bounds); // 获取原始数据包围盒
//      double expansionFactor = 0.1; // 扩展系数（10% 的空余）
//      for (int i = 0; i < 6; i += 2) {
//          double length = bounds[i+1] - bounds[i];
//          bounds[i] -= length * expansionFactor;
//          bounds[i+1] += length * expansionFactor;
//      }
//      cutPlaneRep->PlaceWidget(bounds); // 设置带空余的包围盒

    cutPlaneRep->SetPlaceFactor(1.2);
    cutPlaneRep->PlaceWidget(data->GetBounds());
    cutPlaneRep->SetEdgeColor(1.0,0.0,0.0);
    cutPlaneWidget->SetInteractor(this->m_qvtkInteractor);
    cutPlaneWidget->SetRepresentation(cutPlaneRep);
    cutPlaneWidget->On();
    // 禁止用户缩放或移动包围盒
       cutPlaneRep->SetScaleEnabled(false);     // 禁用缩放
       //cutPlaneRep->SetTranslationEnabled(false); // 禁用平移
       //cutPlaneRep->SetRotationEnabled(false);  // 禁用旋转
    return QString::fromStdString(name);
}
bool TecplotWidget::EnableSliceInteraction(QString sliceName)
{
    std::string name = sliceName.toStdString();

    // 检查切片是否存在
    if (m_sliceWigetList.count(name) == 0 ||
        m_slicePlaneRepList.count(name) == 0 ||
        m_cutterList.count(name) == 0) {
        return false;
    }

    // 获取对应的widget和representation
    vtkImplicitPlaneWidget2* widget = m_sliceWigetList[name];
    vtkImplicitPlaneRepresentation* planeRep = m_slicePlaneRepList[name];

    // 激活交互功能
    widget->ProcessEventsOn();      // 允许处理鼠标/键盘事件
    widget->SetEnabled(1);          // 确保控件可见且激活
    planeRep->SetInteractionState(vtkImplicitPlaneRepresentation::Moving); // 进入可交互状态

    // 取消方向锁定（允许用户自由调整平面方向）
    planeRep->SetNormalToXAxis(false);
    planeRep->SetNormalToYAxis(false);
    planeRep->SetNormalToZAxis(false);

    // 强制渲染更新
    m_renderWindow->Render();

    return true;
}
bool TecplotWidget::HideSliceWidget(QString sliceName)
{
    std::string name = sliceName.toStdString();
    if (m_sliceWigetList.count(name) == 0) {
        return false;
    }

    vtkImplicitPlaneWidget2* widget = m_sliceWigetList[name];
    m_sliceWidgetVisibilityStatus[name]=false;
    widget->SetEnabled(0);      // 禁用并隐藏交互器控件
    m_renderWindow->Render();   // 立即刷新渲染
    return true;
}
bool TecplotWidget::ShowSliceWidget(QString sliceName)
{
    std::string name = sliceName.toStdString();
    if (m_sliceWigetList.count(name) == 0) {
        return false;
    }

    vtkImplicitPlaneWidget2* widget = m_sliceWigetList[name];
    widget->SetEnabled(1);      // 启用并显示交互器控件
    m_sliceWidgetVisibilityStatus[name] = true; // 更新状态
    m_renderWindow->Render();   // 立即刷新渲染
    return true;
}
bool TecplotWidget::Slice(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        return false;
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    vtkCutter* cutter = this->m_cutterList[name];
    vtkActor* actor = this->m_actorsList[name];

    vtkSmartPointer<vtkPlane> cutPlane = vtkSmartPointer<vtkPlane>::New();
    rep->GetPlane(cutPlane);
    cutter->SetCutFunction(cutPlane);
    cutter->Update();
    actor->GetMapper()->SetInputConnection(cutter->GetOutputPort());
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SliceByXPlane(QString sliceName, double xValue)
{
    std::string name = sliceName.toStdString();

    // 检查slice是否存在
    if (this->m_sliceWigetList.count(name) == 0 ||
        this->m_slicePlaneRepList.count(name) == 0 ||
        this->m_cutterList.count(name) == 0) {
//        qWarning() << "Slice not found:" << sliceName;
        return false;
    }

    // 获取相关对象
    vtkImplicitPlaneWidget2* widget = this->m_sliceWigetList[name]; // 获取对应的widget
    vtkImplicitPlaneRepresentation* planeRep = this->m_slicePlaneRepList[name];
    vtkCutter* cutter = this->m_cutterList[name];
    vtkActor* sliceActor = this->m_actorsList[name];
    // 禁用widget的交互
    //widget->SetEnabled(0); // 不可交互，但交互器的显示也不存在了
    widget->ProcessEventsOff();  // 禁止处理鼠标/键盘事件
    //planeRep->SetInteractionState(vtkImplicitPlaneRepresentation::Outside); // 可选：强制进入非交互状态
    // 获取当前平面位置
    double currentOrigin[3];
    planeRep->GetOrigin(currentOrigin);

    // 只更新X坐标，保持Y和Z不变
    planeRep->SetOrigin(xValue, currentOrigin[1], currentOrigin[2]);
    planeRep->SetNormal(1, 0, 0); // 确保是YZ平面
    planeRep->SetNormalToXAxis(true); // 锁定X轴方向

    // 更新切割器
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    planeRep->GetPlane(plane);
    cutter->SetCutFunction(plane);
    cutter->Update();

    // 更新mapper
    vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(sliceActor->GetMapper());
    mapper->SetInputConnection(cutter->GetOutputPort());

    // 渲染更新
    this->m_renderWindow->Render();

    return true;
}
bool TecplotWidget::SliceByYPlane(QString sliceName, double yValue)
{
    std::string name = sliceName.toStdString();

    // 检查slice是否存在
    if (this->m_sliceWigetList.count(name) == 0 ||
        this->m_slicePlaneRepList.count(name) == 0 ||
        this->m_cutterList.count(name) == 0) {
//        qWarning() << "Slice not found:" << sliceName;
        return false;
    }

    // 获取相关对象
    vtkImplicitPlaneWidget2* widget = this->m_sliceWigetList[name]; // 获取对应的widget
    vtkImplicitPlaneRepresentation* planeRep = this->m_slicePlaneRepList[name];
    vtkCutter* cutter = this->m_cutterList[name];
    vtkActor* sliceActor = this->m_actorsList[name];
    // 禁用widget的交互
    //widget->SetEnabled(0); // 不可交互，但交互器的显示也不存在了
    widget->ProcessEventsOff();  // 禁止处理鼠标/键盘事件
    //planeRep->SetInteractionState(vtkImplicitPlaneRepresentation::Outside); // 可选：强制进入非交互状态

    // 获取当前平面位置
    double currentOrigin[3];
    planeRep->GetOrigin(currentOrigin);

    // 只更新Y坐标，保持X和Z不变
    planeRep->SetOrigin(currentOrigin[0],yValue, currentOrigin[2]);
    planeRep->SetNormal(0,1, 0); // 确保是YZ平面
    planeRep->SetNormalToYAxis(true); // 锁定Y轴方向

    // 更新切割器
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    planeRep->GetPlane(plane);
    cutter->SetCutFunction(plane);
    cutter->Update();

    // 更新mapper
    vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(sliceActor->GetMapper());
    mapper->SetInputConnection(cutter->GetOutputPort());

    // 渲染更新
    this->m_renderWindow->Render();

    return true;
}
bool TecplotWidget::SliceByZPlane(QString sliceName, double zValue)
{
    std::string name = sliceName.toStdString();

    // 检查slice是否存在
    if (this->m_sliceWigetList.count(name) == 0 ||
        this->m_slicePlaneRepList.count(name) == 0 ||
        this->m_cutterList.count(name) == 0) {
//        qWarning() << "Slice not found:" << sliceName;
        return false;
    }

    // 获取相关对象
    vtkImplicitPlaneWidget2* widget = this->m_sliceWigetList[name]; // 获取对应的widget
    vtkImplicitPlaneRepresentation* planeRep = this->m_slicePlaneRepList[name];
    vtkCutter* cutter = this->m_cutterList[name];
    vtkActor* sliceActor = this->m_actorsList[name];
    // 禁用widget的交互
    //widget->SetEnabled(0); // 不可交互，但交互器的显示也不存在了
    widget->ProcessEventsOff();  // 禁止处理鼠标/键盘事件
    //planeRep->SetInteractionState(vtkImplicitPlaneRepresentation::Outside); // 可选：强制进入非交互状态
    // 获取当前平面位置
    double currentOrigin[3];
    planeRep->GetOrigin(currentOrigin);

    // 只更新Z坐标，保持X和Y不变
    planeRep->SetOrigin(currentOrigin[0], currentOrigin[1],zValue);
    planeRep->SetNormal(0, 0, 1); // 确保是YZ平面
    planeRep->SetNormalToZAxis(true); // 锁定Z轴方向

    // 更新切割器
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    planeRep->GetPlane(plane);
    cutter->SetCutFunction(plane);
    cutter->Update();

    // 更新mapper
    vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(sliceActor->GetMapper());
    mapper->SetInputConnection(cutter->GetOutputPort());

    // 渲染更新
    this->m_renderWindow->Render();

    return true;
}


QVector3D TecplotWidget::GetSliceOrigin(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        std::cerr<<"error";
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    double origin[3];
    rep->GetNormal(origin);

    QVector3D sliceNormal(static_cast<float>(origin[0]),static_cast<float>(origin[1]),static_cast<float>(origin[2]));
    return sliceNormal;
}
QVector3D TecplotWidget::GetSliceNormal(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        std::cerr<<"error";
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    double normal[3];
    rep->GetNormal(normal);

    QVector3D sliceNormal(static_cast<float>(normal[0]),static_cast<float>(normal[1]),static_cast<float>(normal[2]));
    return sliceNormal;
}

QString TecplotWidget::AddContour(QString contourDerivedActor)
{
    if (m_actorsList.find(contourDerivedActor.toStdString()) == m_actorsList.end()) {
            qWarning() << "Actor" << contourDerivedActor << "does not exist!";
            return "";
     }
    //默认命名方式,此时只是完成了contour的搭建和管理，但是并未设置某个contour的具体值
    this->m_contourNum++;
    std::string contourName = "Contour" + std::to_string(this->m_contourNum);
    Contour* contour = new Contour();
    this->m_contoursList[contourName] = contour; //一个名字contourname对应一个vtkcontour指针去管理
    std::string derivedName = contourDerivedActor.toStdString();
    contour->Initialize(contourName,derivedName,this->m_actorsList,this->m_renderer);
    //this->m_actorsStatus[contourName] = true;
    QString name = QString::fromStdString(contourName);
    return name;
}
double* TecplotWidget::SetContouredBy(QString contourName,QString propertyName)
{
//    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
//    double* range = contourptr->SetActiveProperty(propertyName.toStdString());
//    return range;
    auto it = m_contoursList.find(contourName.toStdString());
        if (it == m_contoursList.end()) {
            qWarning() << "Contour" << contourName << "does not exist!";
            return nullptr;
        }
        Contour* contour = it->second;
        double* range = contour->SetActiveProperty(propertyName.toStdString());
        if (range) {
            m_actorsStatus[contourName.toStdString()] = true; // 设置可见
            m_renderWindow->Render(); // 触发渲染
        }
        return range;
}
int TecplotWidget::AddEntry(QString contourName, double value)
{
//    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
//    int entryId =contourptr->AddEntry(value);
//    return entryId;
    auto it = m_contoursList.find(contourName.toStdString());
        if (it == m_contoursList.end()) {
            qWarning() << "Contour" << contourName << "does not exist!";
            return -1;
        }
        Contour* contour = it->second;
        int entryId = contour->AddEntry(value);
        if (entryId >= 0) {
            m_actorsStatus[contourName.toStdString()] = true; // 确保可见
            m_renderWindow->Render(); // 更新渲染
        }
        return entryId;
}
bool TecplotWidget::EditEntry(QString contourName, int entryId, double value)
{
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    bool flag = contourptr->EditEntry(entryId,value);
    m_renderWindow->Render();
    return flag;
}
bool TecplotWidget::RemoveEntry(QString contourName, int entryId)
{
    //注意，remove一个entry后，该entry后面的所有id都要往前挪一位
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    bool flag = contourptr->RemoveEntry(entryId);
    m_renderWindow->Render();
    return flag;
}
/*void TecplotWidget::ShowContour(QString contourName)
{
    //注意apply之后，默认会关闭其他所有的actor显示
    for(auto& object:this->m_actorsList)
    {//打开的其他actor关闭
        if(object.first!=contourName.toStdString()){
            vtkActor* actor = object.second;
            actor->VisibilityOff();
            m_actorsStatus[object.first] = false;
        }
    }
    this->m_renderer->Render();
}*/
/****矢量图形化****/
QString TecplotWidget::AddGlyph(QString glyphDerived)
{
    //默认命名方式,此时只是完成了contour的搭建和管理，但是并未设置某个contour的具体值
    this->m_glyphNum++;
    std::string glyphName = "Glyph" + std::to_string(this->m_glyphNum);
    Glyph* glyph = new Glyph();
    this->m_glyphsList[glyphName] = glyph;//指针管理
    std::string derivedName = glyphDerived.toStdString();
    glyph->Initialize(glyphName,derivedName,this->m_actorsList,this->m_renderer);
    //此时，actorslist里添加了这个contourname对应的actor
    this->m_actorsStatus[glyphName] = true;
    QString name = QString::fromStdString(glyphName);
    return name;
}
void TecplotWidget::SetGlyphActiveVector(QString glyphName,QString vectorName)
{
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphVector(vectorName.toStdString());
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceTipLength(QString glyphName, double tipLength)
{
    //箭头尖的长度
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceTipLength(tipLength);
    this->m_renderWindow->Render();
}

void TecplotWidget::SetGlyphSourceTipRadius(QString glyphName, double tipRadius)
{
    //箭头尖的半径
     Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
     glyphptr->SetGlyphSourceTipRadius(tipRadius);
     this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceShaftRadius(QString glyphName,double shaftRadius)
{
    // 箭头杆的半径
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceShaftRadius(shaftRadius);
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceScaleFactor(QString glyphName, double scaleFactor)
{
    //factor大小
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceScaleFactor(scaleFactor);
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphPointsNumber(QString glyphName,int pointsNumber)
{
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphPointsNumber(pointsNumber);
    this->m_renderWindow->Render();
}

void TecplotWidget::CalculateQCriterion(QString actorName)
{
    //只进行了计算并将Q值设置为活动标量，如果要绘制涡结构，需要在获得Q值后去contour
    auto objActor = this->m_actorsList[actorName.toStdString()];
    auto data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());//找到这个数据，然后对每个点做计算
    //计算梯度，得到的结果理由Gradients这个梯度array
    auto gradientFilter = vtkSmartPointer<vtkGradientFilter>::New();
    gradientFilter->SetInputData(data);
    gradientFilter->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    gradientFilter->Update();
    //下面是逐个点计算Q值
    int pointNum = data->GetNumberOfPoints();
    auto QCriterion = vtkSmartPointer<vtkDoubleArray>::New();
    QCriterion->SetNumberOfComponents(1);
    QCriterion->SetNumberOfTuples(pointNum);
    QCriterion->SetName("QCriterion");
    auto gradientsArray = gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients");
    for (vtkIdType i = 0; i < pointNum; ++i)
    {
        double grad[9];
        double result;
        // 获取当前点的梯度
        gradientsArray->GetTuple(i, grad);
        // 计算Q
        result = 0.5 * (-grad[0] * grad[0] - grad[4] * grad[4] - grad[8] * grad[8] - 2 * grad[2] * grad[6] - 2 * grad[5] * grad[7] - 2 * grad[1] * grad[3]);
        // 将投影结果存储在数组中
        QCriterion->SetValue(i, result);
    }
    // 将Q加入pointdata并设置为活动标量
    data->GetPointData()->AddArray(QCriterion);
    //data->GetPointData()->SetActiveScalars("QCriterion");//注意，Q值计算改了这里！！！
}
QString TecplotWidget::AddStreamTracer(QString derivedActor)
{
    this->m_streamTraceNum++;
    std::string name = "StreamTracer"+std::to_string(this->m_streamTraceNum);
    vtkActor* objActor = this->m_actorsList[derivedActor.toStdString()];
    vtkDataSet* data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    if(data->IsA("vtkUnstructuredGrid"))
    {
        vtkSmartPointer<vtkGeometryFilter> geoFilter = vtkSmartPointer<vtkGeometryFilter>::New();
        geoFilter->SetInputData(data);
        geoFilter->Update();
        polyData=geoFilter->GetOutput();
    }else if(data->IsA("vtkPolyData"))
    {
        polyData = vtkPolyData::SafeDownCast(data);
    }
    vtkSmartPointer<vtkPolyDataNormals> normalsFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalsFilter->SetInputData(polyData);
    normalsFilter->ComputePointNormalsOn();
    normalsFilter->ConsistencyOn();
    normalsFilter->SplittingOff();  // 禁止切割几何
    normalsFilter->NonManifoldTraversalOff();  // 关闭非流形拓扑处理
    normalsFilter->Update();
    auto normals = normalsFilter->GetOutput()->GetPointData()->GetNormals();
    //取出速度，做速度投影
    int pointNum = data->GetNumberOfPoints();
    vtkDataArray* velocityArray;
    if(data->GetPointData()->HasArray("vel"))
    {
        velocityArray = data->GetPointData()->GetArray("vel");
    }else
    {
        velocityArray = data->GetPointData()->GetArray("velocity");
    }
    auto projectedVelocity = vtkSmartPointer<vtkDoubleArray>::New();/*设置矢量*/
    projectedVelocity->SetNumberOfComponents(3);
    projectedVelocity->SetNumberOfTuples(pointNum);
    projectedVelocity->SetName("projectedVelocity");
    for (vtkIdType i = 0; i < pointNum; ++i)
    {
        double velocity[3];
        double normal[3];
        double projected[3];
        // 获取当前点的速度和法向量
        velocityArray->GetTuple(i, velocity);
        normals->GetTuple(i, normal);
        double dotProduct = vtkMath::Dot(velocity, normal);
        for (int j = 0; j < 3; ++j) {
            projected[j] = velocity[j] - dotProduct * normal[j];
            //cout << projected[j] << " ";
        }
        //cout << endl;
        // 将投影结果存储在数组中
        projectedVelocity->SetTuple(i, projected);
    }
    data->GetPointData()->AddArray(projectedVelocity);
    data->GetPointData()->SetActiveVectors("projectedVelocity");
    //polyData->GetPointData()->AddArray(normals);
    vtkSmartPointer<vtkMaskPoints> maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
    this->m_streamTraceMaskPointsList[name] =maskPoints;
    maskPoints->SetInputData(data);
    maskPoints->SetOnRatio(100); // Select every 10th point
    maskPoints->RandomModeOn(); // Enable random selection
    maskPoints->SetMaximumNumberOfPoints(1000); // Set maximum number of seed points
    maskPoints->Update();
    //test 流线源点
    //auto sphereSource=vtkSmartPointer<vtkSphereSource>::New();
    //sphereSource->SetRadius(0.0005);
    //sphereSource->Update();
    //auto sourceGlyph=vtkSmartPointer<vtkGlyph3D>::New();
    //sourceGlyph->SetInputConnection(maskPoints->GetOutputPort());
    //sourceGlyph->SetSourceConnection(sphereSource->GetOutputPort());
    //auto sphereMapper=vtkSmartPointer<vtkPolyDataMappeer>::New();
    //sphereMapper->SetInputConnection(sourceGlyph->GetOutputPort());
    //auto sphereActor=vtkSmartPointer<vtkActor>::New();
    //sphereActor->SetMapper(sphereMapper);
    //this->m_renderer->AddActor(sphereActor);
    vtkSmartPointer<vtkStreamTracer> streamTracer = vtkSmartPointer<vtkStreamTracer>::New();
    this->m_streamTraceList[name]=streamTracer;
    streamTracer->SetInputData(data); // 使用计算过的 PolyData 作为输入
    streamTracer->SetSourceConnection(maskPoints->GetOutputPort());
    streamTracer->SetIntegrationDirectionToBoth(); // 设置流线的方向
    streamTracer->SetMaximumPropagation(100); // 设置流线的最大长度
    streamTracer->SetMaximumNumberOfSteps(1000);
    streamTracer->SetIntegrator(vtkSmartPointer<vtkRungeKutta4>::New());
    streamTracer->SetComputeVorticity(true); // 计算旋度
    // 映射流线到图形管道
    vtkSmartPointer<vtkPolyDataMapper> streamlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    streamlineMapper->SetInputConnection(streamTracer->GetOutputPort());
    vtkSmartPointer<vtkActor> streamlineActor = vtkSmartPointer<vtkActor>::New();
    streamlineActor->SetMapper(streamlineMapper);
    //streamlineActor->GetProperty()->BackfaceCullingOff();
    streamlineActor->GetProperty()->LightingOff();


    //test propertylist
//    auto streamData =streamTracer->GetOutput()->GetPointData();
//    int tmp=streamData->GetNumberOfArrays();
//    qInfo()<<"streamData num of pointdata array"<<tmp;
//    qInfo()<<"array name:";
//    for(int i =0;i<tmp;i++)
//    {
//        qInfo()<<streamData->GetArrayName(i);
//    }

    //test z-fighting问题
    //streamlineActor->GetProperty()->SetLineWidth(3.0);
    //streamlineActor->GetProperty()->SetColor(1,0,0);  //？？还是有一部分是黑色的
    streamlineMapper->SetResolveCoincidentTopologyToPolygonOffset();
    //streamlineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(2.0,1);//z-fighting没用
    streamlineMapper->ScalarVisibilityOff(); //？？不关闭颜色映射的时候是：蓝色+黑色，关闭了流线是白色+黑色
    this->m_renderer->AddActor(streamlineActor);
    this->m_actorsList[name]= streamlineActor;
    this->m_actorsStatus[name]=true;
    this->m_renderWindow->Render();
    return QString::fromStdString(name);
}
bool TecplotWidget::SetStreamTracerRatio(QString streamTraceActor,int pointRatio,int maxPointNum)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkMaskPoints* maskPoints = this->m_streamTraceMaskPointsList[name];
    maskPoints->SetOnRatio(pointRatio);
    maskPoints->SetMaximumNumberOfPoints(maxPointNum);
    maskPoints->Modified();
    maskPoints->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerDiretion(QString streamTraceActor,int flag)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    switch(flag)
    {
    case -1:
            tracer->SetIntegrationDirectionToBackward();
        break;
    case 0:
        tracer->SetIntegrationDirectionToBoth();
        break;
    case 1:
        tracer->SetIntegrationDirectionToForward();
        break;
    default:
        return false;
    }
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumPropagation(QString streamTraceActor,double maxPropagation)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumPropagation(maxPropagation);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumNumberOfSteps(QString streamTraceActor,int maxNumberOfSteps)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumNumberOfSteps(maxNumberOfSteps);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumIntegrationStep(QString streamTraceActor,double maxIntegrationStep)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumIntegrationStep(maxIntegrationStep);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerIntegrationStepUnit(QString streamTraceActor,int unit)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    if(unit!=1&&unit!=2) return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetIntegrationStepUnit(unit);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
/**************************************************************************
 * ************************************************************************
 * ******************s1面提取************************************************/
QString TecplotWidget::ExtractS1(QString p1SurfaceName,QString p2SurfaceName,QString inletName,QString outletName,QString shroudName,QString hubName,QString fluidName,double relativeR){
    std::string p1Name=p1SurfaceName.toStdString();
    std::string ugName=fluidName.toStdString();
    if(this->m_actorsList.count(p1Name)==0||this->m_actorsList.count(ugName)==0){
        std::cerr << "Error: Actor not found - P1: " << p1Name
                          << " or Fluid: " << ugName << std::endl;
        return "";
    }
    auto p1=vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[p1Name]->GetMapper()->GetInput());
    auto p2 = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[p2SurfaceName.toStdString()]->GetMapper()->GetInput());
    auto ug=vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[ugName]->GetMapper()->GetInput());
    auto inlet = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[inletName.toStdString()]->GetMapper()->GetInput());
    auto outlet = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[outletName.toStdString()]->GetMapper()->GetInput());
    auto shroud = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[shroudName.toStdString()]->GetMapper()->GetInput());
    auto hub = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[hubName.toStdString()]->GetMapper()->GetInput());
    // clipWithSixSurfaces 结果复用
    std::string cacheKey = "clipped_" + p1Name + "_" + ugName;
    vtkSmartPointer<vtkUnstructuredGrid> clippedUG;
    if (m_clippedCache.count(cacheKey)) {
        clippedUG = m_clippedCache[cacheKey];
        //std::cout << "Reusing cached clipped result." << std::endl;
        } else {
//            // 请根据你的项目逻辑调用获取 6 面：p2、inlet、outlet、shroud、hub
//            auto p2 = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[p2SurfaceName.toStdString()]->GetMapper()->GetInput());
//            auto inlet = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[inletName.toStdString()]->GetMapper()->GetInput());
//            auto outlet = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[outletName.toStdString()]->GetMapper()->GetInput());
//            auto shroud = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[shroudName.toStdString()]->GetMapper()->GetInput());
//            auto hub = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[hubName.toStdString()]->GetMapper()->GetInput());
//            qInfo()<<"test";
//            clippedUG = clipWithSixSurfaces(ug, p1, p2, inlet, outlet, shroud, hub);
//            m_clippedCache[cacheKey] = clippedUG;
        clippedUG= ExtractConnectedRegionWithP1(ug,p1);
        m_clippedCache[cacheKey]=clippedUG;
        qInfo()<<"success clippud";
    }
    //vtkSmartPointer<vtkPolyData> s1Data= s1Extract(p1,inlet,outlet,clippedUG,relativeR);
//    vtkSmartPointer<vtkPolyDataMapper> s1Mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
//    s1Mapper->SetInputData(s1Data);
    vtkSmartPointer<vtkUnstructuredGrid> s1Data= s1Extract(p1,p2,inlet,outlet,shroud,hub,clippedUG,relativeR);
    vtkSmartPointer<vtkDataSetMapper> s1Mapper=vtkSmartPointer<vtkDataSetMapper>::New();
    s1Mapper->SetInputData(s1Data);
    vtkSmartPointer<vtkActor> s1Actor=vtkSmartPointer<vtkActor>::New();
    s1Actor->SetMapper(s1Mapper);
    //auto s1Name=QString("S1RelativeR=%1%").arg(relativeR);  // 版本1：使用默认格式
    QString s1Name = QString("S1_%1_RelativeR=%2%").arg(p1SurfaceName).arg(relativeR);
    this->m_actorsList[s1Name.toStdString()]=s1Actor;
    this->m_actorsStatus[s1Name.toStdString()]=true;
    this->m_renderer->AddActor(s1Actor);
    this->m_renderWindow->Render();
    return s1Name;
}
QString TecplotWidget::ExtractS2(QString periodSurfaceName,QString fluidName,int numSteps,double startAngle, double endAngle){
  //  qInfo()<<"成功进入ExtractS2函数 ";
    std::string surfaceName = periodSurfaceName.toStdString();
       std::string volumeName = fluidName.toStdString();

       if (this->m_actorsList.count(surfaceName) == 0 || this->m_actorsList.count(volumeName) == 0) {
           std::cerr << "Error: Actor not found - Surface: " << surfaceName
                     << " or Fluid: " << volumeName << std::endl;
           return "";
       }

       // 获取输入数据
       auto periodSurface = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[surfaceName]->GetMapper()->GetInput());
//       vtkSmartPointer<vtkPoints> points = periodSurface->GetPoints();
//       int num_points = points->GetNumberOfPoints();
//       qInfo()<<"num_points:"<<num_points;
       auto volume = vtkUnstructuredGrid::SafeDownCast(this->m_actorsList[volumeName]->GetMapper()->GetInput());
       vtkSmartPointer<vtkGeometryFilter> geometryFilter =vtkSmartPointer<vtkGeometryFilter>::New();
       geometryFilter->SetInputData(periodSurface);
       geometryFilter->Update();

       vtkSmartPointer<vtkPolyData> pName = geometryFilter->GetOutput();
       // 生成 S2 面
       //qInfo()<<"开始调用generate_s2_surface ";
       vtkSmartPointer<vtkPolyData> s2Data = generate_s2_surface(pName, volume, startAngle, endAngle, numSteps);

       // 创建 Mapper 和 Actor
       vtkSmartPointer<vtkPolyDataMapper> s2Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
       s2Mapper->SetInputData(s2Data);

       vtkSmartPointer<vtkActor> s2Actor = vtkSmartPointer<vtkActor>::New();
       s2Actor->SetMapper(s2Mapper);

       // 构造名字，例如：S2_P2_Start=0_End=360_Steps=10
       QString s2Name = QString("S2_%1_Steps=%2_Start=%3_End=%4")
                        .arg(periodSurfaceName)
                        .arg(numSteps)
                        .arg(startAngle)
                        .arg(endAngle);

       this->m_actorsList[s2Name.toStdString()] = s2Actor;
       this->m_actorsStatus[s2Name.toStdString()] = true;
       this->m_renderer->AddActor(s2Actor);
       this->m_renderWindow->Render();

       return s2Name;
}

/*************************S1提取相关**********************************/
vtkSmartPointer<vtkUnstructuredGrid> TecplotWidget::clipWithSixSurfaces(
    vtkSmartPointer<vtkUnstructuredGrid> input,
    vtkSmartPointer<vtkUnstructuredGrid> periodic1,
    vtkSmartPointer<vtkUnstructuredGrid> periodic2,
    vtkSmartPointer<vtkUnstructuredGrid> inlet,
    vtkSmartPointer<vtkUnstructuredGrid> outlet,
    vtkSmartPointer<vtkUnstructuredGrid> shroud,
    vtkSmartPointer<vtkUnstructuredGrid> hub)
{
    //runtime
    QElapsedTimer timer;
    timer.start();
//       auto regionImplicit = vtkSmartPointer<vtkImplicitBoolean>::New();
//           regionImplicit->SetOperationTypeToIntersection();
//       auto polyFilter = [](vtkSmartPointer<vtkUnstructuredGrid> ug, const QString& name) {
//               auto geo = vtkSmartPointer<vtkGeometryFilter>::New();
//               geo->SetInputData(ug);
//               geo->Update();
//               auto poly = geo->GetOutput();
//               qInfo().noquote() << QString("Surface [%1] polys: %2")
//                                    .arg(name)
//                                    .arg(poly->GetNumberOfPolys());
//               return poly;
//           };

//           auto P1Poly = polyFilter(periodic1, "Periodic1");
//           auto P2Poly = polyFilter(periodic2, "Periodic2");
//           auto inletPoly = polyFilter(inlet, "Inlet");
//           auto outletPoly = polyFilter(outlet, "Outlet");
//           auto shroudPoly = polyFilter(shroud, "Shroud");
//           auto hubPoly = polyFilter(hub, "Hub");

//           // 构造隐式距离函数
//           auto addImplicit = [&](vtkPolyData* pd) {
//               auto impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
//               impl->SetInput(pd);
//               regionImplicit->AddFunction(impl);
//           };
//           addImplicit(inletPoly);
//           addImplicit(outletPoly);
//           addImplicit(P1Poly);
//           addImplicit(P2Poly);
//           addImplicit(shroudPoly);
//           addImplicit(hubPoly);

//           auto clipper = vtkSmartPointer<vtkClipDataSet>::New();
//           clipper->SetInputData(input);
//           clipper->SetClipFunction(regionImplicit);
//           clipper->InsideOutOn();
//           clipper->Update();

//           qint64 elapsed = timer.elapsed();
//           qInfo() << "clipWithSixSurfaces done in" << elapsed << "ms";

//           return clipper->GetOutput();
    // 1. 创建隐式函数对象
    auto regionImplicit = vtkSmartPointer<vtkImplicitBoolean>::New();
    regionImplicit->SetOperationTypeToIntersection();  // 所有面都为必须满足

    //全部ug转为polydata
    auto P1Poly = vtkSmartPointer<vtkGeometryFilter>::New();
    P1Poly->SetInputData(periodic1);
    P1Poly->Update();
//    //添加box预先粗剪，要不然速度太慢了
//    double bounds[6];
//    P1Poly->GetOutput()->GetBounds(bounds);
//    auto boxClipper = vtkSmartPointer<vtkBoxClipDataSet>::New();
//    boxClipper->SetInputData(input);
//    boxClipper->SetBoxClip(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
//    boxClipper->Update();

    auto P2Poly = vtkSmartPointer<vtkGeometryFilter>::New();
    P2Poly->SetInputData(periodic2);
    P2Poly->Update();
    auto inletPoly = vtkSmartPointer<vtkGeometryFilter>::New();
    inletPoly->SetInputData(inlet);
    inletPoly->Update();
    auto outletPoly = vtkSmartPointer<vtkGeometryFilter>::New();
    outletPoly->SetInputData(outlet);
    outletPoly->Update();
    auto shroudPoly = vtkSmartPointer<vtkGeometryFilter>::New();
    shroudPoly->SetInputData(shroud);
    shroudPoly->Update();
    auto hubPoly = vtkSmartPointer<vtkGeometryFilter>::New();
    hubPoly->SetInputData(hub);
    hubPoly->Update();
    // === inlet: 保留面之后（正向） ===
    auto inletImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    inletImpl->SetInput(inletPoly->GetOutput());
    regionImplicit->AddFunction(inletImpl);

    // === outlet: 保留面之前（反向） ===
    auto outletImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    outletImpl->SetInput(outletPoly->GetOutput());
    regionImplicit->AddFunction(outletImpl);

    // === periodic1: 保留一侧（正向） ===
    auto periodic1Impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    periodic1Impl->SetInput(P1Poly->GetOutput());
    regionImplicit->AddFunction(periodic1Impl);

    auto periodic2Impl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    periodic2Impl->SetInput(P2Poly->GetOutput());
    regionImplicit->AddFunction(periodic2Impl);

    auto shroudImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    shroudImpl->SetInput(shroudPoly->GetOutput());
    regionImplicit->AddFunction(shroudImpl);

    auto hubImpl = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
    hubImpl->SetInput(hubPoly->GetOutput());
    regionImplicit->AddFunction(hubImpl);

    // 2. 执行 clip 操作
    auto clipper = vtkSmartPointer<vtkClipDataSet>::New();
    clipper->SetInputData(input);
    //clipper->SetInputData(boxClipper->GetOutput());
    clipper->SetClipFunction(regionImplicit);
    clipper->InsideOutOn();  // 保留“封闭盒”内部区域
    clipper->Update();


    // 3. 拷贝结果（可避免依赖 clipper 管线）
//    auto result = vtkSmartPointer<vtkUnstructuredGrid>::New();
//    result->DeepCopy(clipper->GetOutput());
//    qInfo()<<"function return result";
//    return result;
               qint64 elapsed = timer.elapsed();
               qInfo() << "clipWithSixSurfaces done in" << elapsed << "ms";
    return clipper->GetOutput();
}
vtkSmartPointer<vtkUnstructuredGrid> TecplotWidget::ExtractConnectedRegionWithP1(
    vtkSmartPointer<vtkUnstructuredGrid>inputGrid,
    vtkSmartPointer<vtkUnstructuredGrid> p1Data)
{
    // 1. 找到 P1 的中心点（或任意一个点）作为种子点
    vtkSmartPointer<vtkGeometryFilter> geometryFilter =
        vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputData(p1Data);
    geometryFilter->Update();

    vtkSmartPointer<vtkPolyData> p1PolyData = geometryFilter->GetOutput();
    double seedPoint[3];
    p1PolyData->GetPoint(0, seedPoint); // 也可用中心点或平均值

    // 2. 使用 ConnectivityFilter 提取与该点所在区域连通的单元
    auto connectivityFilter = vtkSmartPointer<vtkConnectivityFilter>::New();
    connectivityFilter->SetInputData(inputGrid);
    connectivityFilter->SetExtractionModeToClosestPointRegion();
    connectivityFilter->SetClosestPoint(seedPoint);
    connectivityFilter->Update();

    // 3. 获取输出
    auto result = vtkSmartPointer<vtkUnstructuredGrid>::New();
    result->ShallowCopy(connectivityFilter->GetOutput());

    return result;
}
/***************************************************************************
 ***************************************************************************
 ***************************************************************************
 **********************Contour的实现*****************************************
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************/
void Contour::Initialize(std::string contourName, std::string derivedName, std::map<std::string, vtkActor *> &actorsList, vtkRenderer *renderer)
{
    // 初始化,新建contourfiler、polydatamapper、actor、renderer，但没设置actorsStatus
    this->m_contourFilter = vtkSmartPointer<vtkContourFilter>::New(); //新建一个filter

    vtkActor* derivedActor = actorsList[derivedName];
    this->m_data = vtkDataSet::SafeDownCast(derivedActor->GetMapper()->GetInput());//filter的输入
    this->m_contourFilter->SetInputData(this->m_data);
    this->m_contourFilter->SetNumberOfContours(0);
    this->m_propertyName="";
//    this->m_contourFilter->Update();
//    //filter连接mapper、actor
//    vtkSmartPointer<vtkPolyDataMapper> contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
//    contourMapper->SetInputConnection(this->m_contourFilter->GetOutputPort());
//    contourMapper->ScalarVisibilityOff();
//    vtkSmartPointer<vtkActor> contourActor = vtkSmartPointer<vtkActor>::New();
//    contourActor->SetMapper(contourMapper);
//    actorsList[contourName] = contourActor;
//    renderer->AddActor(contourActor);
    this->m_contourName = contourName;
        this->out_actorsList = &actorsList;
        this->out_renderer = renderer;
        // 不调用 Update()，不创建 Actor
}
double* Contour::SetActiveProperty(std::string propertyName)
{
//    // 设置activescalar
//    this->propertyStatus=true;
//    int scalarId = this->m_data->GetPointData()->SetActiveScalars(propertyName.c_str());
//    this->m_contourFilter->Update();
//    return this->m_data->GetPointData()->GetArray(scalarId)->GetRange();
    if (!m_data->GetPointData()->HasArray(propertyName.c_str())) {
            std::cerr << "Property " << propertyName << " not found in " << m_contourName << std::endl;
            return nullptr;
        }
    this->m_propertyName=propertyName;
        //this->propertyStatus = true;
        //int scalarId = this->m_data->GetPointData()->SetActiveScalars(propertyName.c_str());
//        vtkDataArray* array = this->m_data->GetPointData()->GetArray(scalarId);
    vtkDataArray* array = this->m_data->GetPointData()->GetArray(propertyName.c_str());
    this->m_contourFilter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, propertyName.c_str());
        // 创建并添加 Actor（如果尚未创建）
        if (!m_contourActor) {
            vtkSmartPointer<vtkPolyDataMapper> contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            contourMapper->SetInputConnection(this->m_contourFilter->GetOutputPort());
            contourMapper->ScalarVisibilityOff(); // 默认关闭标量映射
            m_contourActor = vtkSmartPointer<vtkActor>::New();
            m_contourActor->SetMapper(contourMapper);
            (*out_actorsList)[m_contourName] = m_contourActor;
            out_renderer->AddActor(m_contourActor);
        }

        this->m_contourFilter->Update();

        double* range = new double[2];
        array->GetRange(range);
        //qInfo()<<(*range)<<*(range+1);
        return range;
}
int Contour::AddEntry(double value)
{
    if(this->m_propertyName=="") return -1;
    this->m_contourFilter->SetValue(this->m_valueNum,value);
    this->m_valueNum++;//指向下一个空的entry位置
    this->m_contourFilter->SetNumberOfContours(this->m_valueNum); // 显式设置等值数量
    this->m_contourFilter->Update();
    this->m_contourFilter->Modified();
    // 调试输出
        std::cout << "Contour " << m_contourName << " - Added value: " << value
                  << ", Number of contours: " << this->m_contourFilter->GetNumberOfContours() << std::endl;
        for (int i = 0; i < this->m_valueNum; ++i) {
            std::cout << "Contour value " << i << ": " << this->m_contourFilter->GetValue(i) << std::endl;
        }
        // 调试：打印 vtkContourFilter 输出的点数据
        // 获取 ContourFilter 的输出
           vtkSmartPointer<vtkPolyData> contourOutput = this->m_contourFilter->GetOutput();
           if (!contourOutput || contourOutput->GetNumberOfPoints() == 0) {
               std::cerr << "Error: ContourFilter output is empty!" << std::endl;
               return -1;
           }

           // 获取 PointData
           vtkSmartPointer<vtkPointData> pointData = contourOutput->GetPointData();
           std::cout << "ContourFilter Output PointData Arrays: " << pointData->GetNumberOfArrays() << std::endl;

           for (int i = 0; i < pointData->GetNumberOfArrays(); ++i) {
               vtkSmartPointer<vtkDataArray> dataArray = pointData->GetArray(i);
               if (dataArray) {
                   std::cout << "Array " << i << ": " << (dataArray->GetName() ? dataArray->GetName() : "NULL");

                   // 获取数据范围
                   double range[2];
                   dataArray->GetRange(range);
                   std::cout << "  | Range: [" << range[0] << ", " << range[1] << "]" << std::endl;
               }
           }

    return this->m_valueNum-1;//id从0开始
}
bool Contour::EditEntry(int entryId,double value)
{
    if(entryId < 0 || entryId >= this->m_valueNum){
        return false;
    }
    this->m_contourFilter->SetValue(entryId,value);
    this->m_contourFilter->SetNumberOfContours(this->m_valueNum); // 确保数量一致
    this->m_contourFilter->Update();
    this->m_contourFilter->Modified();
    return true;
}
bool Contour::RemoveEntry(int entryId)
{
    if(entryId < 0 || entryId >= this->m_valueNum){
        return false;
    }
    std::vector<double> contourValues;
    for (int i = 0; i < this->m_valueNum; ++i)
    {
        if (i != entryId) // 跳过要删除的等值
        {
            contourValues.push_back(this->m_contourFilter->GetValue(i));
        }
    }
    this->m_valueNum--;
    this->m_contourFilter->SetNumberOfContours(this->m_valueNum);
    for (size_t i = 0; i < this->m_valueNum; ++i)
    {
        this->m_contourFilter->SetValue(static_cast<int>(i), contourValues[i]);
    }
    contourValues.clear();
    this->m_contourFilter->Update();
    this->m_contourFilter->Modified();
    return true;
}
/***************************************************************************
 ***************************************************************************
 ***************************************************************************
 **********************Glyph的实现*****************************************
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************/
void Glyph::Initialize(std::string glyphName,std::string derivedName,std::map<std::string,vtkActor*>& actorsList,vtkRenderer* renderer)
{
     // 初始化,新建contourfiler、polydatamapper、actor、renderer，但没设置actorsStatus
    this->m_arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    this->m_maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
    this->m_glyphFilter = vtkSmartPointer<vtkGlyph3D>::New();

    vtkActor* derivedActor = actorsList[derivedName];
    this->m_data = vtkDataSet::SafeDownCast(derivedActor->GetMapper()->GetInput());//maskpoints的输入
    this->m_maskPoints->SetInputData(this->m_data);
    this->m_maskPoints->SetOnRatio(1000);
    this->m_maskPoints->RandomModeOn(); // 随机选择点
    //source设置（这些是默认
    this->m_arrowSource->SetTipLength(0.1);   // 设置箭头尖的长度
    this->m_arrowSource->SetTipRadius(0.01);  // 设置箭头尖的半径
    this->m_arrowSource->SetShaftRadius(0.001); // 设置箭头杆的半径
    //source和采样点接入filter
    this->m_glyphFilter->SetSourceConnection(this->m_arrowSource->GetOutputPort());
    this->m_glyphFilter->SetInputConnection(this->m_maskPoints->GetOutputPort());
    this->m_glyphFilter->SetVectorModeToUseVector();
    this->m_glyphFilter->SetScaleModeToScaleByVector();
    this->m_glyphFilter->SetScaleFactor(0.0001); // 调整箭头大小,原速度适配factor=0.00001
    //filter连接mapper、actor
    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(this->m_glyphFilter->GetOutputPort());
    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    actorsList[glyphName] = glyphActor;
    renderer->AddActor(glyphActor);
}
void Glyph::SetGlyphVector(std::string vectorName)
{
    // 打印点总数
      //vtkIdType numPoints = this->m_data->GetNumberOfPoints();
      //std::cout << "Total Points: " << numPoints << std::endl;

      // 打印所有PointData数组名称
      vtkPointData* pd = this->m_data->GetPointData();
//      std::cout << "PointData Arrays: ";
//      for (int i = 0; i < pd->GetNumberOfArrays(); ++i) {
//          std::cout << pd->GetArrayName(i) << " ";
//      }
//      std::cout << std::endl;

      // 检查目标向量是否存在
      vtkDataArray* vectors = pd->GetArray(vectorName.c_str());
      if (!vectors) {
          std::cerr << "Error: Vector array '" << vectorName << "' not found!" << std::endl;
          return;
      }

      // 打印向量维度
     // std::cout << "Vector dimensions: " << vectors->GetNumberOfComponents() << std::endl;

      // 打印向量范围
//      double range[2];
//      vectors->GetRange(range, -1); // -1表示计算所有分量的总体范围
//      std::cout << "Vector range: [" << range[0] << ", " << range[1] << "]" << std::endl;

    this->m_data->GetPointData()->SetActiveVectors(vectorName.c_str());
    this->m_data->Modified();
}
void Glyph::SetGlyphSourceTipLength(double tipLength)
{
    this->m_arrowSource->SetTipLength(tipLength);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceTipRadius(double tipRadius)
{
    this->m_arrowSource->SetTipRadius(tipRadius);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceShaftRadius(double shaftRadius)
{
    this->m_arrowSource->SetShaftRadius(shaftRadius);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceScaleFactor(double scaleFactor)
{
    this->m_glyphFilter->SetScaleFactor(scaleFactor);
    this->m_glyphFilter->Modified();
    this->m_glyphFilter->Update();
}
void Glyph::SetGlyphPointsNumber(int pointsNumber)
{
    int n = this ->m_data->GetNumberOfPoints();
    int ratio = n/pointsNumber;
    this->m_maskPoints->SetOnRatio(ratio);
    this->m_maskPoints->Modified();
    this->m_arrowSource->Update();
}
void TecplotReader::pointsReader(int pointId, const std::string& line, int varNum, std::vector<vtkSmartPointer<vtkFloatArray>>& zoneData, vtkPoints* thePoints) {
    std::istringstream iss(line);
    std::string theValue;
    float x, y, z, tmp;
    for (int i = 0; i < varNum; i++)
    {
        iss >> theValue;
        tmp = stod(theValue);
        //std::cout << tmp << " ";
        zoneData[i]->SetValue(pointId, tmp);
        switch (i)
        {
        case 0:
            x = tmp;
            break;
        case 1:
            y = tmp;
            break;
        case 2:
            z = tmp;
            break;
        default:
            break;
        }
    }
    thePoints->InsertNextPoint(x, y, z);
}

void TecplotReader::cellsReader(const std::string& cellType, const std::string& line, vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid) {
    std::istringstream iss(line);
    std::string tmp;
    std::vector<int> realcell;
    auto ids = vtkSmartPointer<vtkIdList>::New();
    int cur = 0;
    while (iss >> tmp)
    {
        int theID = stoi(tmp) - 1;

        if (realcell.empty())
        {
            realcell.push_back(theID);
            ids->InsertNextId(theID);
        }
        else if (theID != realcell[cur]) {
            realcell.push_back(theID);
            cur++;
            ids->InsertNextId(theID);
        }
        //std::cout << "theID : " << theID << std::endl;
    }
    int len = realcell.size();
    switch (len)
    {
    case 3:
        unstructuredGrid->InsertNextCell(VTK_TRIANGLE, ids);
        break;
    case 4:
        if (cellType == "FEQUADRILATERAL")
            unstructuredGrid->InsertNextCell(VTK_QUAD, ids);
        else
            unstructuredGrid->InsertNextCell(VTK_TETRA, ids);
        break;
    case 5:
        unstructuredGrid->InsertNextCell(VTK_PYRAMID, ids);
        break;
    case 6:
        unstructuredGrid->InsertNextCell(VTK_WEDGE, ids);
        break;
    case 8:
        unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, ids);
        break;
    default:
        break;
    }
}
/***************************************************************************************
 *****************************数据读入tecplotreader具体实现********************************
 ***************************************************************************************/
//vtkSmartPointer<vtkUnstructuredGrid> manualRemoveOverlap(vtkSmartPointer<vtkUnstructuredGrid> inputGrid, double tolerance) {
//    if (!inputGrid) {
//        std::cerr << "Error: Input grid is null!" << std::endl;
//        return nullptr;
//    }

//    vtkPoints* points = inputGrid->GetPoints();
//    if (!points) {
//        std::cerr << "Error: Points object is null!" << std::endl;
//        return nullptr;
//    }

//    vtkSmartPointer<vtkDataArray> oldPoints = points->GetData();
//    if (!oldPoints) {
//        std::cerr << "Error: Failed to get point data!" << std::endl;
//        return nullptr;
//    }
//    int numOldPoints = oldPoints->GetNumberOfTuples();
//    std::cout << "Point data type: " << oldPoints->GetDataTypeAsString() << std::endl;

//    // 阶段1: 构建旧点到新点的索引映射
//    using HashKey = std::tuple<int, int, int>;
//    std::map<HashKey, int> hashBuckets;
//    std::vector<int> pointMap(numOldPoints);
//    std::vector<double> newPointsList;

//    double point[3];
//    for (int oldIdx = 0; oldIdx < numOldPoints; oldIdx++) {
//        oldPoints->GetTuple(oldIdx, point);
//        HashKey hashKey(
//            static_cast<int>(std::round(point[0] / tolerance)),
//            static_cast<int>(std::round(point[1] / tolerance)),
//            static_cast<int>(std::round(point[2] / tolerance))
//        );

//        if (hashBuckets.find(hashKey) == hashBuckets.end()) {
//            int newIdx = newPointsList.size() / 3;
//            newPointsList.push_back(point[0]);
//            newPointsList.push_back(point[1]);
//            newPointsList.push_back(point[2]);
//            hashBuckets[hashKey] = newIdx;
//            pointMap[oldIdx] = newIdx;
//        }
//        else {
//            pointMap[oldIdx] = hashBuckets[hashKey];
//        }
//    }

//    // 阶段2: 重建去重后的点集
//    vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
//    vtkSmartPointer<vtkDoubleArray> newPointsData = vtkSmartPointer<vtkDoubleArray>::New();
//    newPointsData->SetNumberOfComponents(3);
//    newPointsData->SetNumberOfTuples(newPointsList.size() / 3);
//    for (size_t i = 0; i < newPointsList.size() / 3; i++) {
//        newPointsData->SetTuple(i, &newPointsList[i * 3]);
//    }
//    newPoints->SetData(newPointsData);
//    std::cout << "New points created: " << newPoints->GetNumberOfPoints() << std::endl;

//    // 阶段3: 重建单元并过滤重复单元
//    vtkSmartPointer<vtkCellArray> cells = inputGrid->GetCells();
//    if (!cells) {
//        std::cerr << "Error: No cells in input grid!" << std::endl;
//        return nullptr;
//    }

//    vtkSmartPointer<vtkDataArray> offsets = cells->GetOffsetsArray();
//    vtkSmartPointer<vtkDataArray> connectivity = cells->GetConnectivityArray();
//    if (!offsets) {
//        std::cerr << "Error: Failed to get offsets array!" << std::endl;
//        return nullptr;
//    }
//    if (!connectivity) {
//        std::cerr << "Error: Failed to get connectivity array!" << std::endl;
//        return nullptr;
//    }

//    std::cout << "Offsets data type: " << offsets->GetDataTypeAsString() << std::endl;
//    std::cout << "Connectivity data type: " << connectivity->GetDataTypeAsString() << std::endl;

//    std::vector<vtkIdType> newConnectivity(connectivity->GetNumberOfTuples());
//    vtkSmartPointer<vtkUnsignedCharArray> oldCellTypes = inputGrid->GetCellTypesArray();
//    std::vector<unsigned char> newCellTypes;

//    for (vtkIdType i = 0; i < connectivity->GetNumberOfTuples(); i++) {
//        newConnectivity[i] = pointMap[static_cast<int>(connectivity->GetTuple1(i))];
//    }

//    using CellTuple = std::vector<vtkIdType>;
//    std::set<CellTuple> cellSet;
//    std::vector<CellTuple> uniqueCells;

//    for (vtkIdType i = 0; i < offsets->GetNumberOfTuples() - 1; i++) {
//        vtkIdType start = static_cast<vtkIdType>(offsets->GetTuple1(i));
//        vtkIdType end = static_cast<vtkIdType>(offsets->GetTuple1(i + 1));
//        if (start < 0 || end > static_cast<vtkIdType>(newConnectivity.size()) || start >= end) {
//            std::cerr << "Error: Invalid cell offsets at index " << i << std::endl;
//            continue;
//        }
//        CellTuple cellTuple(newConnectivity.begin() + start, newConnectivity.begin() + end);
//        if (cellSet.find(cellTuple) == cellSet.end()) {
//            cellSet.insert(cellTuple);
//            uniqueCells.push_back(cellTuple);
//            newCellTypes.push_back(oldCellTypes->GetValue(i));
//        }
//    }
//    std::cout << "Unique cells created: " << uniqueCells.size() << std::endl;

//    // 阶段4: 构建输出网格
//    vtkSmartPointer<vtkUnstructuredGrid> outputGrid =
//        vtkSmartPointer<vtkUnstructuredGrid>::New();
//    outputGrid->SetPoints(newPoints);

//    vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
//    for (const auto& cell : uniqueCells) {
//        cellArray->InsertNextCell(cell.size(), cell.data());
//    }

//    vtkSmartPointer<vtkUnsignedCharArray> cellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
//    cellTypes->SetNumberOfTuples(uniqueCells.size());
//    for (size_t i = 0; i < uniqueCells.size(); i++) {
//        cellTypes->SetValue(i, newCellTypes[i]);
//    }

//    outputGrid->SetCells(cellTypes, cellArray);
//    std::cout << "Output grid cells set: " << outputGrid->GetNumberOfCells() << std::endl;

//    std::cout << "Before return - Points: " << outputGrid->GetNumberOfPoints() << std::endl;
//    std::cout << "Before return - Cells: " << outputGrid->GetNumberOfCells() << std::endl;

//    return outputGrid;
//}
vtkSmartPointer<vtkUnstructuredGrid> manualRemoveOverlap(vtkSmartPointer<vtkUnstructuredGrid> inputGrid, double tolerance) {
    if (!inputGrid) {
        std::cerr << "Error: Input grid is null!" << std::endl;
        return nullptr;
    }

    vtkPoints* points = inputGrid->GetPoints();
    if (!points) {
        std::cerr << "Error: Points object is null!" << std::endl;
        return nullptr;
    }

    vtkSmartPointer<vtkDataArray> oldPoints = points->GetData();
    if (!oldPoints) {
        std::cerr << "Error: Failed to get point data!" << std::endl;
        return nullptr;
    }
    int numOldPoints = oldPoints->GetNumberOfTuples();
    //std::cout << "Point data type: " << oldPoints->GetDataTypeAsString() << std::endl;

    // 获取原始的 PointData
    vtkSmartPointer<vtkPointData> oldPointData = inputGrid->GetPointData();
    if (!oldPointData) {
        std::cerr << "Warning: No point data in input grid!" << std::endl;
    }

    // 阶段1: 构建旧点到新点的索引映射
    using HashKey = std::tuple<int, int, int>;
    std::map<HashKey, int> hashBuckets;
    std::vector<int> pointMap(numOldPoints);
    std::vector<double> newPointsList;

    double point[3];
    for (int oldIdx = 0; oldIdx < numOldPoints; oldIdx++) {
        oldPoints->GetTuple(oldIdx, point);
        HashKey hashKey(
            static_cast<int>(std::round(point[0] / tolerance)),
            static_cast<int>(std::round(point[1] / tolerance)),
            static_cast<int>(std::round(point[2] / tolerance))
        );

        if (hashBuckets.find(hashKey) == hashBuckets.end()) {
            int newIdx = newPointsList.size() / 3;
            newPointsList.push_back(point[0]);
            newPointsList.push_back(point[1]);
            newPointsList.push_back(point[2]);
            hashBuckets[hashKey] = newIdx;
            pointMap[oldIdx] = newIdx;
        } else {
            pointMap[oldIdx] = hashBuckets[hashKey];
        }
    }

    // 阶段2: 重建去重后的点集
    vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> newPointsData = vtkSmartPointer<vtkDoubleArray>::New();
    newPointsData->SetNumberOfComponents(3);
    newPointsData->SetNumberOfTuples(newPointsList.size() / 3);
    for (size_t i = 0; i < newPointsList.size() / 3; i++) {
        newPointsData->SetTuple(i, &newPointsList[i * 3]);
    }
    newPoints->SetData(newPointsData);
    //std::cout << "New points created: " << newPoints->GetNumberOfPoints() << std::endl;

    // 阶段3: 重建去重后的 PointData，排除 "vtkOriginalPointIds"
    vtkSmartPointer<vtkPointData> newPointData = vtkSmartPointer<vtkPointData>::New();
    if (oldPointData && oldPointData->GetNumberOfArrays() > 0) {
        int numNewPoints = newPoints->GetNumberOfPoints();
        for (int arrayIdx = 0; arrayIdx < oldPointData->GetNumberOfArrays(); arrayIdx++) {
            vtkDataArray* oldArray = oldPointData->GetArray(arrayIdx);
            if (!oldArray) continue;

            // 跳过 "vtkOriginalPointIds"
            if (oldArray->GetName() && strcmp(oldArray->GetName(), "vtkOriginalPointIds") == 0) {
                //std::cout << "Skipping vtkOriginalPointIds array" << std::endl;
                continue;
            }

            // 创建新的数据数组
            vtkSmartPointer<vtkDataArray> newArray = vtkSmartPointer<vtkDataArray>::Take(
                vtkDataArray::CreateDataArray(oldArray->GetDataType()));
            newArray->SetNumberOfComponents(oldArray->GetNumberOfComponents());
            newArray->SetNumberOfTuples(numNewPoints);
            newArray->SetName(oldArray->GetName());

            // 根据 pointMap 填充新数组
            std::vector<bool> pointAssigned(numNewPoints, false);
            for (int oldIdx = 0; oldIdx < numOldPoints; oldIdx++) {
                int newIdx = pointMap[oldIdx];
                if (!pointAssigned[newIdx]) {
                    newArray->SetTuple(newIdx, oldArray->GetTuple(oldIdx));
                    pointAssigned[newIdx] = true;
                }
            }

            // 将新数组添加到 newPointData
            newPointData->AddArray(newArray);
        }
    }

    // 阶段4: 重建单元并过滤重复单元
    vtkSmartPointer<vtkCellArray> cells = inputGrid->GetCells();
    if (!cells) {
        std::cerr << "Error: No cells in input grid!" << std::endl;
        return nullptr;
    }

    vtkSmartPointer<vtkDataArray> offsets = cells->GetOffsetsArray();
    vtkSmartPointer<vtkDataArray> connectivity = cells->GetConnectivityArray();
    if (!offsets || !connectivity) {
        std::cerr << "Error: Failed to get offsets or connectivity array!" << std::endl;
        return nullptr;
    }

    //std::cout << "Offsets data type: " << offsets->GetDataTypeAsString() << std::endl;
    //std::cout << "Connectivity data type: " << connectivity->GetDataTypeAsString() << std::endl;

    std::vector<vtkIdType> newConnectivity(connectivity->GetNumberOfTuples());
    vtkSmartPointer<vtkUnsignedCharArray> oldCellTypes = inputGrid->GetCellTypesArray();
    std::vector<unsigned char> newCellTypes;

    for (vtkIdType i = 0; i < connectivity->GetNumberOfTuples(); i++) {
        newConnectivity[i] = pointMap[static_cast<int>(connectivity->GetTuple1(i))];
    }

    using CellTuple = std::vector<vtkIdType>;
    std::set<CellTuple> cellSet;
    std::vector<CellTuple> uniqueCells;

    for (vtkIdType i = 0; i < offsets->GetNumberOfTuples() - 1; i++) {
        vtkIdType start = static_cast<vtkIdType>(offsets->GetTuple1(i));
        vtkIdType end = static_cast<vtkIdType>(offsets->GetTuple1(i + 1));
        if (start < 0 || end > static_cast<vtkIdType>(newConnectivity.size()) || start >= end) {
            std::cerr << "Error: Invalid cell offsets at index " << i << std::endl;
            continue;
        }
        CellTuple cellTuple(newConnectivity.begin() + start, newConnectivity.begin() + end);
        if (cellSet.find(cellTuple) == cellSet.end()) {
            cellSet.insert(cellTuple);
            uniqueCells.push_back(cellTuple);
            newCellTypes.push_back(oldCellTypes->GetValue(i));
        }
    }
    //std::cout << "Unique cells created: " << uniqueCells.size() << std::endl;

    // 阶段5: 构建输出网格
    vtkSmartPointer<vtkUnstructuredGrid> outputGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    outputGrid->SetPoints(newPoints);
    if (newPointData && newPointData->GetNumberOfArrays() > 0) {
        outputGrid->GetPointData()->DeepCopy(newPointData);
    }

    vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
    for (const auto& cell : uniqueCells) {
        cellArray->InsertNextCell(cell.size(), cell.data());
    }

    vtkSmartPointer<vtkUnsignedCharArray> cellTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
    cellTypes->SetNumberOfTuples(uniqueCells.size());
    for (size_t i = 0; i < uniqueCells.size(); i++) {
        cellTypes->SetValue(i, newCellTypes[i]);
    }

    outputGrid->SetCells(cellTypes, cellArray);
    //std::cout << "Output grid cells set: " << outputGrid->GetNumberOfCells() << std::endl;

    //std::cout << "Before return - Points: " << outputGrid->GetNumberOfPoints() << std::endl;
    //std::cout << "Before return - Cells: " << outputGrid->GetNumberOfCells() << std::endl;

    return outputGrid;
}
void processZone(vtkSmartPointer<vtkUnstructuredGrid> inputGrid,
                 vtkSmartPointer<vtkUnstructuredGrid>& outputGrid,
                 double tolerance, std::mutex& mtx) {
    try {
        if (!inputGrid || !inputGrid->GetPoints() || inputGrid->GetNumberOfCells() == 0) {
            std::cerr << "Invalid input grid in processZone!" << std::endl;
            outputGrid = nullptr;
            return;
        }

        vtkSmartPointer<vtkRemoveUnusedPoints> removeFilter = vtkSmartPointer<vtkRemoveUnusedPoints>::New();
        removeFilter->SetInputData(inputGrid);
        removeFilter->Update();
        outputGrid = manualRemoveOverlap(removeFilter->GetOutput(), tolerance);

        if (!outputGrid) {
            std::cerr << "manualRemoveOverlap returned nullptr!" << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lock(mtx);
        //std::cout << "Processed zone - Points: " << outputGrid->GetNumberOfPoints() << ", Cells: " << outputGrid->GetNumberOfCells() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in processZone: " << e.what() << std::endl;
        outputGrid = nullptr;
    }
}
vtkMultiBlockDataSet* TecplotReader::ReadTecplotData(const std::string &fileName){
    clock_t start_time = clock();

    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " <<fileName << std::endl;
        return nullptr;
    }

    std::string line;
    std::string currentToken;

    std::string title;
    int solutiontime = -1; // 默认值表示未设置
    int zoneNum = 0;
    int nodeNum = 0;

    std::vector<std::string> varName;
    std::vector<std::string> zoneTitle;
    std::vector<int> zoneCellNum;
    std::vector<std::string> zoneCellType;

    auto thePoints = vtkSmartPointer<vtkPoints>::New();
    std::vector<vtkSmartPointer<vtkFloatArray>> zoneData;
    //auto multiBlock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    auto multiBlock = vtkMultiBlockDataSet::New();
    auto sharedPointData = vtkSmartPointer<vtkPointData>::New();
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>> rawGrids;

    std::vector<char> buffer(1024 * 1024);
    file.rdbuf()->pubsetbuf(buffer.data(), buffer.size());
    std::cin.tie(nullptr);
    std::ios_base::sync_with_stdio(false);

    bool pointsOK = false;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (!pointsOK) {
            if (line.find("TITLE") != std::string::npos||line.find("ITLE") != std::string::npos) {
                int pos1 = line.find("\"");
                int pos2 = line.find("\"", pos1 + 1);
                title = line.substr(pos1 + 1, pos2 - pos1 - 1);
                continue;
            }

            if (line.find("VARIABLES") != std::string::npos) {
                size_t pos1 = line.find("\"");
                size_t pos2 = line.find("\"", pos1 + 1);
                std::string tempVarName;
                while (pos2 != std::string::npos) {
                    tempVarName = line.substr(pos1 + 1, pos2 - pos1 - 1);
                    varName.push_back(tempVarName);
                    if (pos2 == line.size() - 1) break;
                    pos1 = line.find("\"", pos2 + 1);
                    pos2 = line.find("\"", pos1 + 1);
                }
                continue;
            }

            if (line.find("solutiontime") != std::string::npos && line.find("ZONE") == std::string::npos) {
                std::istringstream iss(line);
                while (iss >> currentToken) {
                    if (std::isdigit(currentToken[0])) {
                        solutiontime = std::stoi(currentToken);
                        break;
                    }
                }
                continue;
            }

            if (line.find("ZONE") != std::string::npos) {
                zoneNum++;
                std::replace(line.begin(), line.end(), '=', ' ');
                std::replace(line.begin(), line.end(), ',', ' ');
                std::replace(line.begin(), line.end(), '"', ' ');
                std::istringstream iss(line);

                while (iss >> currentToken) {
                    if (currentToken == "T") {
                        iss >> currentToken;
                        zoneTitle.push_back(currentToken);
                    }
                    else if (currentToken == "N") {
                        iss >> nodeNum;
                    }
                    else if (currentToken == "E") {
                        iss >> currentToken;
                        zoneCellNum.push_back(std::stoi(currentToken));
                    }
                    else if (currentToken == "ZONETYPE") {
                        iss >> currentToken;
                        zoneCellType.push_back(currentToken);
                    }
                    else if (currentToken == "solutiontime") {
                        iss >> currentToken;
                        if (std::isdigit(currentToken[0])) {
                            solutiontime = std::stoi(currentToken);
                        }
                    }
                }

                // 读取下一行，应该是数据行
                while (std::getline(file, line) && !line.empty()) {
                    std::istringstream iss(line);
                    iss >> currentToken;

                    if (line.find("solutiontime") != std::string::npos) {
                        while (iss >> currentToken) {
                            if (std::isdigit(currentToken[0])) {
                                solutiontime = std::stoi(currentToken);
                                break;
                            }
                        }
                        continue; // 读取下一行
                    }

                    if (currentToken.find("ZONE") == std::string::npos &&
                        (std::isdigit(currentToken[0]) || currentToken[0] == '-')) {
                        // 是数据行，开始读取点数据
                        int varNum = varName.size();
                        for (int i = 0; i < varNum; i++) {
                            vtkFloatArray* theArray = vtkFloatArray::New();
                            theArray->SetNumberOfTuples(nodeNum);
                            theArray->SetName(varName[i].c_str());
                            zoneData.push_back(theArray);
                            theArray->Delete();
                        }
                        //qInfo()<<"varNum:"<<varNum;
                        //qInfo()<<"ZoneDataSize"<<zoneData.size();
                        pointsReader(0, line, varNum, zoneData, thePoints);

                        for (int i = 1; i < nodeNum; i++) {
                            std::getline(file, line);
                            pointsReader(i, line, varNum, zoneData, thePoints);
                        }
                        pointsOK = true;
                        break; // 数据读取完成，退出循环
                    }
                    else {
                        std::cerr << "Unexpected line after ZONE: " << line << std::endl;
                    }
                }
                continue;
            }
            continue;
        }

        if (line.find("ZONE") != std::string::npos) {
            zoneNum++;
            std::replace(line.begin(), line.end(), '=', ' ');
            std::replace(line.begin(), line.end(), '"', ' ');
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream iss(line);

            while (iss >> currentToken) {
                if (currentToken == "T") {
                    iss >> currentToken;
                    zoneTitle.push_back(currentToken);
                }
                else if (currentToken == "E") {
                    iss >> currentToken;
                    zoneCellNum.push_back(std::stoi(currentToken));
                }
                else if (currentToken == "ZONETYPE") {
                    iss >> currentToken;
                    zoneCellType.push_back(currentToken);
                }
            }
            continue;
        }

        auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        ug->SetPoints(thePoints);

        if (zoneNum == 1) {
            bool hasVelocity = false;
            for (auto& data : zoneData) {
                if (strcmp(data->GetName(), "vel") == 0 || strcmp(data->GetName(), "velocity") == 0)
                    hasVelocity = true;
                ug->GetPointData()->AddArray(data);
            }
            if (!hasVelocity) {
                auto calculator = vtkSmartPointer<vtkArrayCalculator>::New();
                calculator->SetInputData(ug);
                calculator->AddScalarArrayName("u");
                calculator->AddScalarArrayName("v");
                calculator->AddScalarArrayName("w");
                calculator->SetResultArrayName("velocity");
                calculator->SetFunction("u*iHat + v*jHat + w*kHat");
                calculator->Update();
                vtkDataArray* velocityArray = calculator->GetUnstructuredGridOutput()->GetPointData()->GetArray("velocity");
                ug->GetPointData()->AddArray(velocityArray);
            }
            sharedPointData = ug->GetPointData();
        }
        else {
            ug->GetPointData()->DeepCopy(sharedPointData);
        }

        int zoneId = zoneNum - 1;
        cellsReader(zoneCellType[zoneId], line, ug);
        for (int i = 1; i < zoneCellNum[zoneId]; i++) {
            std::getline(file, line);
            cellsReader(zoneCellType[zoneId], line, ug);
        }
//        vtkSmartPointer<vtkRemoveUnusedPoints> removeFilter = vtkSmartPointer<vtkRemoveUnusedPoints>::New();
//        removeFilter->SetInputData(ug);
//        removeFilter->Update();
//        auto outputGrid = manualRemoveOverlap(removeFilter->GetOutput());
        //multiBlock->SetBlock(zoneId, outputGrid);
        rawGrids.push_back(ug);
        multiBlock->SetBlock(zoneId, ug);
        multiBlock->GetMetaData(zoneId)->Set(vtkCompositeDataSet::NAME(), zoneTitle[zoneId].c_str());
    }

    file.close();
    // 统一处理所有 zone 的去重
    std::vector<vtkSmartPointer<vtkUnstructuredGrid>> cleanedGrids(zoneNum);
    std::mutex mtx;

    std::vector<std::thread> threads;
    for (int i = 0; i < zoneNum; i++) {
        threads.emplace_back(processZone, rawGrids[i], std::ref(cleanedGrids[i]), 1e-6, std::ref(mtx));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int i = 0; i < zoneNum; i++) {
        if (cleanedGrids[i]) {
            multiBlock->SetBlock(i, cleanedGrids[i]);
        }
        else {
            std::cerr << "Failed to process zone " << i << std::endl;
        }
    }

    //clock_t end_time = clock();
    //std::cout << "总处理时间（含去重）：" << (end_time - start_time) / (double)CLOCKS_PER_SEC << "s" << std::endl;
    //qInfo()<<multiBlock->GetNumberOfBlocks();
    //qInfo()<<vtkUnstructuredGrid::SafeDownCast(multiBlock->GetBlock(0))->GetPointData()->GetNumberOfArrays();
    return multiBlock;
}
//vtkSmartPointer<vtkMultiBlockDataSet> TecplotReader::ReadTecplotData(const std::string &fileName) {
//    //clock_t start_time = clock();

//    std::ifstream file(fileName);
//    if (!file.is_open()) {
//        std::cerr << "Failed to open file: " << fileName << std::endl;
//        return nullptr;
//    }

//    std::string line;
//    std::string currentToken;

//    std::string title;
//    int solutiontime = -1;
//    int zoneNum = 0;
//    int nodeNum = 0; //the number of points

//    std::vector<std::string> varName;
//    std::vector<std::string> zoneTitle;
//    std::vector<int> zoneCellNum;
//    std::vector<std::string> zoneCellType;

//    auto thePoints = vtkSmartPointer<vtkPoints>::New();
//    std::vector<vtkSmartPointer<vtkFloatArray>> zoneData;
//    vtkSmartPointer<vtkMultiBlockDataSet> multiBlock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
//    auto sharedPointData = vtkSmartPointer<vtkPointData>::New();
//    std::vector<vtkSmartPointer<vtkUnstructuredGrid>> rawGrids;

//    std::vector<char> buffer(1024 * 1024);
//    file.rdbuf()->pubsetbuf(buffer.data(), buffer.size());
//    std::cin.tie(nullptr);
//    std::ios_base::sync_with_stdio(false);

//    bool pointsOK = false;
//    while (std::getline(file, line))
//    {
//        if (line.empty()) continue;
//        if (!pointsOK)
//        {
//            if (line.find("TITLE") != std::string::npos||line.find("ITLE") != std::string::npos)
//            {
//                int pos1 = line.find("\"");
//                int pos2 = line.find("\"", pos1 + 1);
//                title = line.substr(pos1 + 1, pos2 - pos1 - 1);
//                continue;
//            }

//            if (line.find("VARIABLES") != std::string::npos)
//            {
//                size_t pos1 = line.find("\"");
//                size_t pos2 = line.find("\"", pos1 + 1);
//                std::string tempVarName;
//                while (pos2 != std::string::npos) {
//                    tempVarName = line.substr(pos1 + 1, pos2 - pos1 - 1);
//                    varName.push_back(tempVarName);
//                    if (pos2 == line.size() - 1) break;
//                    pos1 = line.find("\"", pos2 + 1);
//                    pos2 = line.find("\"", pos1 + 1);
//                }
//                continue;
//            }

//            if (line.find("solutiontime") != std::string::npos&& line.find("ZONE") == std::string::npos)
//            {
//                std::istringstream iss(line);
//                while (iss >> currentToken) {
//                    if (isdigit(currentToken[0]))
//                    {
//                        solutiontime = std::stoi(currentToken);
//                        break;
//                    }
//                }
//                continue; //
//            }

//            if (line.find("ZONE") != std::string::npos)
//            {
//                zoneNum++;
//                std::replace(line.begin(), line.end(), '=', ' ');
//                std::replace(line.begin(), line.end(), ',', ' ');
//                std::replace(line.begin(), line.end(), '"', ' ');
//                std::istringstream iss(line);

//                while (iss >> currentToken)
//                {
//                    if (currentToken == "T")
//                    {
//                        iss >> currentToken;
//                        //std::cout <<"T=" <<currentToken << std::endl;
//                        zoneTitle.push_back(currentToken);
//                    }
//                    else if (currentToken == "N")
//                    {
//                        iss >> nodeNum;
//                        //std::cout <<"N="<< nodeNum << std::endl;
//                    }
//                    else if (currentToken == "E")
//                    {
//                        iss >> currentToken;
//                        //std::cout <<"E="<< currentToken << std::endl;
//                        zoneCellNum.push_back(std::stoi(currentToken));
//                    }
//                    else if (currentToken == "ZONETYPE")
//                    {
//                        iss >> currentToken;
//                        zoneCellType.push_back(currentToken);
//                    } else if (currentToken == "solutiontime") {
//                        iss >> currentToken;
//                        if (std::isdigit(currentToken[0])) {
//                            solutiontime = std::stoi(currentToken);
//                        }
//                    }
//                }
//                // 读取下一行，应该是数据行
//                while (std::getline(file, line) && !line.empty()) {
//                    std::istringstream iss(line);
//                    iss >> currentToken;

//                    if (line.find("solutiontime") != std::string::npos) {
//                        while (iss >> currentToken) {
//                            if (std::isdigit(currentToken[0])) {
//                                solutiontime = std::stoi(currentToken);
//                                break;
//                            }
//                        }
//                        continue; // 读取下一行
//                    }

//                    if (currentToken.find("ZONE") == std::string::npos &&
//                        (std::isdigit(currentToken[0]) || currentToken[0] == '-')) {
//                        // 是数据行，开始读取点数据
//                        int varNum = varName.size();
//                        for (int i = 0; i < varNum; i++) {
//                            vtkFloatArray* theArray = vtkFloatArray::New();
//                            theArray->SetNumberOfTuples(nodeNum);
//                            theArray->SetName(varName[i].c_str());
//                            zoneData.push_back(theArray);
//                            theArray->Delete();
//                        }
//                        pointsReader(0, line, varNum, zoneData, thePoints);

//                        for (int i = 1; i < nodeNum; i++) {
//                            std::getline(file, line);
//                            pointsReader(i, line, varNum, zoneData, thePoints);
//                        }
//                        pointsOK = true;
//                        break; // 数据读取完成，退出循环
//                    }
//                    else {
//                        std::cerr << "Unexpected line after ZONE: " << line << std::endl;
//                    }
//                }
//                continue;
//            }

////            int varNum = varName.size();

////            for (int i = 0; i < varNum; i++) {
////                vtkFloatArray* theArray = vtkFloatArray::New();
////                theArray->SetNumberOfTuples(nodeNum);
////                theArray->SetName(varName[i].c_str());
////                zoneData.push_back(theArray);
////                theArray->Delete();
////            }
////            pointsReader(0, line, varNum, zoneData, thePoints);


////            for (int i = 1; i < nodeNum; i++)
////            {
////                std::getline(file, line);
////                pointsReader(i, line, varNum, zoneData, thePoints);
////            }
////            pointsOK = true;
////            continue;
////        }
//        }
//        if (line.find("ZONE") != std::string::npos)
//        {
//            zoneNum++;

//            std::replace(line.begin(), line.end(), '=', ' ');
//            std::replace(line.begin(), line.end(), '"', ' ');
//            std::replace(line.begin(), line.end(), ',', ' ');
//            std::istringstream iss(line);
//            //std::cout << line << std::endl;
//            while (iss >> currentToken)
//            {
//                if (currentToken == "T")
//                {
//                    iss >> currentToken;
//                    //std::cout << currentToken << std::endl;
//                    zoneTitle.push_back(currentToken);
//                }
//                else if (currentToken == "E")
//                {
//                    iss >> currentToken;
//                    //std::cout << currentToken << std::endl;
//                    zoneCellNum.push_back(std::stoi(currentToken));
//                }
//                else if (currentToken == "ZONETYPE")
//                {
//                    iss >> currentToken;
//                    zoneCellType.push_back(currentToken);
//                }
//            }
//            continue;
//        }

//        auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
//        ug->SetPoints(thePoints);

//        if (zoneNum == 1) {
//            bool hasVelocity = false;
//            for (auto& data : zoneData) {
//                if(strcmp(data->GetName(), "vel") == 0 || strcmp(data->GetName(), "velocity") == 0)
//                    hasVelocity = true;
//                ug->GetPointData()->AddArray(data);
//            }
//            if(!hasVelocity)
//            {
//                auto calculator=vtkSmartPointer<vtkArrayCalculator>::New();
//                //如果没有velocity属性，就进行添加
//                calculator->SetInputData(ug);
//                calculator->AddScalarArrayName("u");
//                calculator->AddScalarArrayName("v");
//                calculator->AddScalarArrayName("w");
//                calculator->SetResultArrayName("velocity");
//                calculator->SetFunction("u*iHat + v*jHat + w*kHat");
//                calculator->Update();
//                vtkDataArray* velocityArray = calculator->GetUnstructuredGridOutput()->GetPointData()->GetArray("velocity");
//                ug->GetPointData()->AddArray(velocityArray);
//            }
//            sharedPointData = ug->GetPointData();
//        }
//        else {
//            //ug->GetPointData()->ShallowCopy(sharedPointData);
//            ug->GetPointData()->DeepCopy(sharedPointData);
//        }

//        int zoneId = zoneNum - 1;
//        //std::cout << "zoneId = " << zoneId << std::endl;
//        cellsReader(zoneCellType[zoneId], line, ug);
//        for (int i = 1; i < zoneCellNum[zoneId]; i++)
//        {
//            std::getline(file, line);
//            cellsReader(zoneCellType[zoneId], line, ug);
//        }

//        rawGrids.push_back(ug);
//        multiBlock->SetBlock(zoneId, ug);
//        multiBlock->GetMetaData(zoneId)->Set(vtkCompositeDataSet::NAME(), zoneTitle[zoneId].c_str());
//    }
//     file.close();
////     // 统一处理所有 zone 的去重
////     std::vector<vtkSmartPointer<vtkUnstructuredGrid>> cleanedGrids(zoneNum);
////     std::mutex mtx;

////     std::vector<std::thread> threads;
////     for (int i = 0; i < zoneNum; i++) {
////         threads.emplace_back(processZone, rawGrids[i], std::ref(cleanedGrids[i]), 1e-6, std::ref(mtx));
////     }

////     for (auto& thread : threads) {
////         thread.join();
////     }

////     for (int i = 0; i < zoneNum; i++) {
////         if (cleanedGrids[i]) {
////             multiBlock->SetBlock(i, cleanedGrids[i]);
////         }
////         else {
////             std::cerr << "Failed to process zone " << i << std::endl;
////         }
////     }
////    //clock_t end_time = clock();
////    //std::cout << "" << (end_time - start_time) / (double)CLOCKS_PER_SEC << "s" << std::endl;
////     try {
////             auto multiBlock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
////             // ... 数据读取 ...
////             std::cout << "Before threading - Blocks: " << multiBlock->GetNumberOfBlocks() << std::endl;
////             for (int i = 0; i < zoneNum; i++) {
////                 threads.emplace_back(processZone, rawGrids[i], std::ref(cleanedGrids[i]), 1e-6, std::ref(mtx));
////             }
////             for (auto& thread : threads) {
////                 thread.join();
////             }
////             std::cout << "After threading - Blocks: " << multiBlock->GetNumberOfBlocks() << std::endl;
////             return multiBlock;
////         } catch (const std::exception& e) {
////             std::cerr << "Error in ReadTecplotData: " << e.what() << std::endl;
////             return nullptr;
////         }
//    return multiBlock;
//}

