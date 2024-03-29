/* \author Aaron Brown */
// Create simple 3d highway enviroment using PCL
// for exploring self-driving car sensors

#include "processPointClouds.h"
#include "render/render.h"
#include "sensors/lidar.h"
// using templates for processPointClouds so also include .cpp to help linker
#include "processPointClouds.cpp"

std::vector<Car> initHighway(bool renderScene,
    pcl::visualization::PCLVisualizer::Ptr &viewer) {

    Car egoCar(Vect3(0, 0, 0), Vect3(4, 2, 2), Color(0, 1, 0), "egoCar");
    Car car1(Vect3(15, 0, 0), Vect3(4, 2, 2), Color(0, 0, 1), "car1");
    Car car2(Vect3(8, -4, 0), Vect3(4, 2, 2), Color(0, 0, 1), "car2");
    Car car3(Vect3(-12, 4, 0), Vect3(4, 2, 2), Color(0, 0, 1), "car3");

    std::vector<Car> cars;
    cars.push_back(egoCar);
    cars.push_back(car1);
    cars.push_back(car2);
    cars.push_back(car3);

    if (renderScene) {
        renderHighway(viewer);
        egoCar.render(viewer);
        car1.render(viewer);
        car2.render(viewer);
        car3.render(viewer);
    }

    return cars;
}

void simpleHighway(pcl::visualization::PCLVisualizer::Ptr &viewer) {
    // ----------------------------------------------------
    // -----Open 3D viewer and display simple highway -----
    // ----------------------------------------------------

    // RENDER OPTIONS
    bool renderScene = false;
    std::vector<Car> cars = initHighway(renderScene, viewer);

    // TODO:: Create lidar sensor
    Lidar *lidar = new Lidar(cars, 0);
    pcl::PointCloud<pcl::PointXYZ>::Ptr points = lidar->scan();
    // renderRays(viewer, lidar->position, points);
    // renderPointCloud(viewer, points, "Rays");
    ProcessPointClouds<pcl::PointXYZ> solver;
    std::pair<pcl::PointCloud<pcl::PointXYZ>::Ptr,
        pcl::PointCloud<pcl::PointXYZ>::Ptr>
        segmentResult = solver.SegmentPlane(points, 100, 0.2f);

    std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> cloudClusters =
        solver.Clustering(segmentResult.second, 1.0, 3, 30);

    int clusterId = 0;
    std::vector<Color> colors = { Color(1, 0, 0), Color(0, 1, 0), Color(0, 0, 1) };

    for (pcl::PointCloud<pcl::PointXYZ>::Ptr cluster : cloudClusters) {
        std::cout << "cluster size ";
        solver.numPoints(cluster);
        renderPointCloud(viewer, cluster, "obstCloud" + std::to_string(clusterId),
            colors[clusterId]);
        ++clusterId;
    }
}

void cityBlock(pcl::visualization::PCLVisualizer::Ptr &viewer,
    ProcessPointClouds<pcl::PointXYZI> solver,
    pcl::PointCloud<pcl::PointXYZI>::Ptr environment) {
    pcl::PointCloud<pcl::PointXYZI>::Ptr afterFiltered =
        solver.FilterCloud(environment, 0.5f, Eigen::Vector4f(-10, -5, -3, 1),
            Eigen::Vector4f(30, 5, 5, 1));

    std::pair<pcl::PointCloud<pcl::PointXYZI>::Ptr,
        pcl::PointCloud<pcl::PointXYZI>::Ptr>
        segmentResult = solver.SegmentPlaneMine(afterFiltered, 100, 0.2f);

    renderPointCloud(viewer, segmentResult.first, "1", Color(0.f, 1.f, 0.f));
    //renderPointCloud(viewer, segmentResult.second, "2", Color(1.f, 0.f, 0.f));

    std::vector<pcl::PointCloud<pcl::PointXYZI>::Ptr> cloudClusters =
        solver.ClusteringMine(segmentResult.second, 2, 10, 2000);

    int clusterId = 0;
    std::vector<Color> colors = { Color(1, 0, 0), Color(0, 1, 0), Color(0, 0, 1) };

    for (pcl::PointCloud<pcl::PointXYZI>::Ptr cluster : cloudClusters) {
        //std::cout << "cluster size ";
        //solver.numPoints(cluster);
        renderPointCloud(viewer, cluster,
            "obstCloud" + std::to_string(clusterId),
            colors[clusterId % 3]);
        Box aabbBox = solver.BoundingBox(cluster);
        renderBox(viewer, aabbBox, clusterId, Color(1.f, 0.f, 0.f), 0.5);
        ++clusterId;
    }

}

// setAngle: SWITCH CAMERA ANGLE {XY, TopDown, Side, FPS}
void initCamera(CameraAngle setAngle,
    pcl::visualization::PCLVisualizer::Ptr &viewer) {

    viewer->setBackgroundColor(0, 0, 0);

    // set camera position and angle
    viewer->initCameraParameters();
    // distance away in meters
    int distance = 16;

    switch (setAngle) {
    case XY:
        viewer->setCameraPosition(-distance, -distance, distance, 1, 1, 0);
        break;
    case TopDown:
        viewer->setCameraPosition(0, 0, distance, 1, 0, 1);
        break;
    case Side:
        viewer->setCameraPosition(0, -distance, 0, 0, 0, 1);
        break;
    case FPS:
        viewer->setCameraPosition(-10, 0, 0, 0, 0, 1);
    }

    if (setAngle != FPS)
        viewer->addCoordinateSystem(1.0);
}

int main(int argc, char **argv) {
    std::cout << "starting enviroment" << std::endl;

    pcl::visualization::PCLVisualizer::Ptr viewer(
        new pcl::visualization::PCLVisualizer("3D Viewer"));
    CameraAngle setAngle = XY;
    initCamera(setAngle, viewer);
    //simpleCityBlock(viewer);
    // simpleHighway(viewer);

    ProcessPointClouds<pcl::PointXYZI> solver;
    pcl::PointCloud<pcl::PointXYZI>::Ptr inputCloudI;
    std::vector<boost::filesystem::path> stream =
        solver.streamPcd("../src/sensors/data/pcd/data_1");
    auto streamIterator = stream.begin();

    while (!viewer->wasStopped()) {

        // Clear viewer
        viewer->removeAllPointClouds();
        viewer->removeAllShapes();

        // Load pcd and run obstacle detection process
        inputCloudI = solver.loadPcd((*streamIterator).string());
        cityBlock(viewer, solver, inputCloudI);

        streamIterator++;
        if (streamIterator == stream.end())
            streamIterator = stream.begin();

        viewer->spinOnce();
    }
}