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


void ProcessOneFile(std::string filepath)
{
    // std::cout << "Running in " << std::filesystem::current_path() << std::endl;
    std::string content = ReadFile(filepath);
    webifc::utility::LoaderSettings set;
    set.COORDINATE_TO_ORIGIN = true;
    webifc::utility::LoaderErrorHandler errorHandler;
    webifc::schema::IfcSchemaManager schemaManager;
    webifc::parsing::IfcLoader loader(set.TAPE_SIZE, set.MEMORY_LIMIT, errorHandler, schemaManager);

    auto start = ms();
    loader.LoadFile([&](char* dest, size_t sourceOffset, size_t destSize)
        {
            uint32_t length = std::min(content.size() - sourceOffset, destSize);
            memcpy(dest, &content[sourceOffset], length);
            return length;
        });
    // std::ofstream outputStream("D:/web-ifc/benchmark/ifcfiles/output.ifc");
    // outputStream << loader.DumpAsIFC();
    // exit(0);
    auto time = ms() - start;
    std::cout << " - Reading took " << time << "ms" << std::endl;

    // std::ofstream outputFile("output.ifc");
    // outputFile << loader.DumpSingleObjectAsIFC(14363);
    // outputFile.close();

    start = ms();
    webifc::geometry::IfcGeometryProcessor geometryLoader(loader,errorHandler,schemaManager,set.CIRCLE_SEGMENTS,set.COORDINATE_TO_ORIGIN);
    auto errors = errorHandler.GetErrors();
    errorHandler.ClearErrors();
    for (auto error : errors)
    {
        std::cout << error.expressID << " " << error.ifcType << " " << std::to_string((int)error.type) << " " << error.message << std::endl;
    }
    time = ms() - start;

    std::cout << " - Generating geometry took " << time << "ms" << std::endl;
}


void Benchmark()
{
    std::vector<BenchMarkResult> results;
    std::string path = "C:/Data/Ifc/_DebugSupport";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().extension().string() != ".ifc")
        {
            continue;
        }

        std::string filePath = entry.path().string();
        std::string filename = entry.path().filename().string();

        std::cout << "Start: processing " << filePath << std::endl;
        auto start = ms();
        {
            ProcessOneFile(filePath);
        }
        auto time = ms() - start;

        BenchMarkResult result;
        result.file = filename;
        result.timeMS = time;
        result.sizeBytes = entry.file_size();
        results.push_back(result);

        std::cout << "End: Processing " << result.file << " took " << time << "ms" << std::endl << std::endl;
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
}



int main()
{
    std::cout << "Starting regression suite main()" << std::endl;
    Benchmark();
    return 0;
}