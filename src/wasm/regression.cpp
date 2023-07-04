/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */


#include <iostream>
#include <fstream>
#include <filesystem>
#include "test/io_helpers.h"

#include "parsing/IfcLoader.h"
#include "schema/IfcSchemaManager.h"
#include "geometry/IfcGeometryProcessor.h"
#include "utility/LoaderError.h"
#include "utility/LoaderSettings.h"
#include "schema/ifc-schema.h"


using namespace webifc::io;

long long ms()
{
    using namespace std::chrono;
    milliseconds millis = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch());

    return millis.count();
}

std::string ReadFile(std::string filename)
{
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

void SpecificLoadTest(webifc::parsing::IfcLoader &loader, webifc::geometry::IfcGeometryProcessor &geometryLoader, uint64_t num)
{
    auto walls = loader.GetExpressIDsWithType(webifc::schema::IFCSLAB);

    bool writeFiles = true;

    auto mesh = geometryLoader.GetMesh(num);

    if (writeFiles)
    {
        DumpMesh(mesh, geometryLoader, "TEST.obj");
    }
}

struct BenchMarkResult
{
    std::string file;
    long long timeMS;
    long long sizeBytes;
};


BenchMarkResult ProcessOneFile(std::filesystem::directory_entry file)
{
    std::string filePath = file.path().string();
    std::cout << "Start: processing " << filePath << std::endl;
    BenchMarkResult result;
    result.file = filePath;
    result.sizeBytes = file.file_size();
    
    webifc::utility::LoaderSettings set;
    set.COORDINATE_TO_ORIGIN = true;
    webifc::utility::LoaderErrorHandler errorHandler;
    webifc::schema::IfcSchemaManager schemaManager;
    webifc::parsing::IfcLoader loader(set.TAPE_SIZE, set.MEMORY_LIMIT, errorHandler, schemaManager);

    auto firstStart = ms();
    std::string content = ReadFile(filePath);
    loader.LoadFile([&](char* dest, size_t sourceOffset, size_t destSize)
        {
            uint32_t length = std::min(content.size() - sourceOffset, destSize);
            memcpy(dest, &content[sourceOffset], length);
            return length;
        });
    // std::ofstream outputStream("D:/web-ifc/benchmark/ifcfiles/output.ifc");
    // outputStream << loader.DumpAsIFC();
    // exit(0);
    auto time = ms() - firstStart;
    std::cout << " - Reading and loading took " << time << "ms." << std::endl;

    // std::ofstream outputFile("output.ifc");
    // outputFile << loader.DumpSingleObjectAsIFC(14363);
    // outputFile.close();
    int tallyEntities = 0;
    int errorEntities = 0;
    auto geomStart = ms();
    webifc::geometry::IfcGeometryProcessor geometryLoader(loader,errorHandler,schemaManager,set.CIRCLE_SEGMENTS,set.COORDINATE_TO_ORIGIN, true);
    // std::vector<webifc::geometry::IfcFlatMesh> meshes;
    for (auto type : schemaManager.GetIfcElementList())
    {
        auto elements = loader.GetExpressIDsWithType(type);
        for (uint32_t i = 0; i < elements.size(); i++)
        {
            tallyEntities++;
            webifc::geometry::IfcFlatMesh mesh = geometryLoader.GetFlatMesh(elements[i]);
            for (auto& geom : mesh.geometries)
            {
                auto& flatGeom = geometryLoader.GetGeometry(geom.geometryExpressID);
                flatGeom.GetVertexData();
            }
            if (errorHandler.GetErrors().size() > 0)
            {
                errorEntities++;
                errorHandler.ClearErrors();
            }
            geometryLoader.Clear(); // removing the data
        }
    }
    auto endtime = ms();
    
    std::cout << " - Generating geometry took " << endtime - geomStart << "ms." << std::endl;
    std::cout << " - " << errorEntities << " errors and " << tallyEntities << " entities." << std::endl;
    result.timeMS = time;
    std::cout << " - Total processing took " << endtime - firstStart << "ms." << std::endl << std::endl;
    return result;
}

int Benchmark(char *argPath)
{
    std::string path(argPath);
    if (!std::filesystem::exists(path))
    {
        std::cout << "Path '" <<  path << "' not found." << std::endl;
        return 1;
    }

    std::vector<BenchMarkResult> results;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file() || entry.path().extension().string() != ".ifc")
        {
            continue;
        }       
        auto result = ProcessOneFile(entry);
        results.push_back(result);
    }

    std::cout << std::endl;
    std::cout << "Results:" << std::endl;
    double avgMBsec = 0;
    for (auto& result : results)
    {
        double MBsec = result.sizeBytes / 1000.0 / result.timeMS;
        avgMBsec += MBsec;
        std::cout << " - " << result.file << ": " << MBsec << " MB/sec" << std::endl;
    }
    avgMBsec /= results.size();

    std::cout << std::endl;
    std::cout << "Average: " << avgMBsec << " MB/sec" << std::endl;
    std::cout << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1 || argc >= 1 && (strcmp(argv[1],"help")==0))
    {
        std::cout << "No argument passed to regression suite main()" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << " - regression benchmark <folder> (performance evaluation of ifc files in folder)" << std::endl;
        std::cout << " - regression help (prints this help)" << std::endl;
        return 1;
    }
    if (argc > 1 && strcmp(argv[1],"benchmark") == 0)
    {
        if (argc == 2)
        {
            std::cout << "The benchmark option requires a further parameter identifying the folder to process." << std::endl;
            return 1;
        }
        return Benchmark(argv[2]);
    }
    std::cout << "Invalid command line options, use 'regression help' for instructions." << std::endl;
    return 0;
}