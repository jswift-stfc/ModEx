#include "nexusFile.h"
#include "processors.h"
#include "window.h"
#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <fmt/core.h>
#include <optional>
#include <vector>

int main(int argc, char **argv)
{
    // Paths to NeXuS files to process
    std::vector<std::string> inputFiles_;
    // Output directory
    std::string outputDirectory_;
    // Processing mode
    Processors::ProcessingMode processingMode_ = Processors::ProcessingMode::None;
    // Window definition
    std::string windowName_{"np_output"};
    double windowStartTime_{0.0};
    double windowWidth_{0.0};
    double windowOffset_{0.0};
    bool relativeStartTime_{false};
    // Time between windows
    double windowDelta_{0.0};
    // Number of slices to partition window in to
    int windowSlices_{1};
    // Target spectrum for event get (optional)
    int targetIndex_;

    // Define and parse CLI arguments
    CLI::App app("NeXuS Processor (np), Copyright (C) 2024 Jared Swift and Tristan Youngs.");
    // -- Window Definition
    app.add_option("-n,--name", windowName_, "Name of the window, used as a prefix to all output files")
        ->group("Window Definition");
    app.add_option("-s,--start", windowStartTime_,
                   "Absolute start time of the window (i.e. seconds since epoch). Specify --relative-time if using a time "
                   "relative to the run start.")
        ->group("Window Definition");
    app.add_option("-w,--width", windowWidth_, "Window width in seconds)")->group("Window Definition");
    app.add_flag("--relative-start", relativeStartTime_,
                 "Flag that the given window start time is relative to the first run start time rather than absolute (seconds "
                 "since epoch)")
        ->group("Window Definition");
    app.add_option("-d,--delta", windowDelta_, "Time between window occurrences, in seconds")->group("Window Definition");
    app.add_option("--offset", windowOffset_, "Time after start time, in seconds, that the window begins.")
        ->group("Window Definition");
    app.add_option("-l,--slices", windowSlices_, "Number of slices to split window definition in to (default = 1, no slicing)")
        ->group("Window Definition");
    // -- Input Files
    app.add_option("-f,--files", inputFiles_, "List of NeXuS files to process")->group("Input Files")->required();
    // -- Output Files
    app.add_option("--output-dir", outputDirectory_, "Output directory for generated NeXuS files.")->group("Output Files");
    // -- Processing Modes
    app.add_option_function<int>(
           "--dump-events",
           [&](int id)
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::DumpEvents;
               targetIndex_ = id;
           },
           "Dump all events for specified detector index")
        ->group("Processing");
    app.add_option_function<int>(
           "--print-events",
           [&](int id)
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::PrintEvents;
               targetIndex_ = id;
           },
           "Print all events for specified detector index")
        ->group("Processing");
    app.add_option_function<int>(
           "--dump-detector",
           [&](int id)
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::DumpDetector;
               targetIndex_ = id;
           },
           "Dump specified detector histogram")
        ->group("Processing");
    app.add_option_function<int>(
           "--dump-monitor",
           [&](int id)
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::DumpMonitor;
               targetIndex_ = id;
           },
           "Dump specified monitor histogram")
        ->group("Processing");
    app.add_flag_callback(
           "--summed",
           [&]()
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::PartitionEventsSummed;
           },
           "Sum windows / slices over all files and output NeXuS files per-slice")
        ->group("Processing");
    app.add_flag_callback(
           "--individual",
           [&]()
           {
               if (processingMode_ != Processors::ProcessingMode::None)
               {
                   fmt::print("Error: Multiple processing modes given.\n");
                   throw(CLI::RuntimeError());
               }
               processingMode_ = Processors::ProcessingMode::PartitionEventsIndividual;
           },
           "Output NeXuS files for each window / slice")
        ->group("Processing");
    // -- Post Processing
    app.add_flag_callback(
           "--scale-monitors", [&]() { Processors::postProcessingMode_ = Processors::PostProcessingMode::ScaleMonitors; },
           "Scale monitor counts in final output to match the number of frames processed for detectors")
        ->group("Post-Processing");
    app.add_flag_callback(
           "--scale-detectors", [&]() { Processors::postProcessingMode_ = Processors::PostProcessingMode::ScaleDetectors; },
           "Scale detector counts in final output to match the number of frames used for monitor counts")
        ->group("Post-Processing");

    CLI11_PARSE(app, argc, argv);

    // Perform processing
    switch (processingMode_)
    {
        case (Processors::ProcessingMode::None):
            fmt::print("No processing mode specified. Basic data from files will be shown.\n");
            for (const auto &file : inputFiles_)
            {
                NeXuSFile nxs(file, true);
                nxs.prepareSpectraSpace(true);
            }
            break;
        case (Processors::ProcessingMode::DumpEvents):
        case (Processors::ProcessingMode::PrintEvents):
            Processors::dumpEventTimesEpoch(inputFiles_, targetIndex_,
                                            processingMode_ == Processors::ProcessingMode::PrintEvents);
            break;
        case (Processors::ProcessingMode::DumpDetector):
            Processors::dumpDetector(inputFiles_, targetIndex_);
            break;
        case (Processors::ProcessingMode::DumpMonitor):
            Processors::dumpMonitor(inputFiles_, targetIndex_);
            break;
        case (Processors::ProcessingMode::PartitionEventsIndividual):
        case (Processors::ProcessingMode::PartitionEventsSummed):
            // Sanity check
            if ((windowWidth_ + windowOffset_) > windowDelta_)
            {
                fmt::print("Error: Window width (including any optional offset) is greater than window delta.\n");
                return 1;
            }
            if (windowSlices_ < 1)
            {
                fmt::print("Error: Invalid number of window slices provided ({}).\n", windowSlices_);
                return 1;
            }

            // Construct the master window definition
            if (relativeStartTime_)
            {
                // Need to query first NeXuS file to get its start time
                if (inputFiles_.empty())
                {
                    fmt::print("Error: Need at least one input NeXuS file.");
                    return 1;
                }
                NeXuSFile firstFile(inputFiles_.front());
                fmt::print("Window start time converted from relative to absolute time: {} => {} (= {} + {})\n",
                           windowStartTime_, windowStartTime_ + firstFile.startSinceEpoch(), firstFile.startSinceEpoch(),
                           windowStartTime_);
                windowStartTime_ += firstFile.startSinceEpoch();
            }
            fmt::print("Window start time (including any offset) is {}.\n", windowStartTime_ + windowOffset_);

            if (processingMode_ == Processors::ProcessingMode::PartitionEventsIndividual)
                Processors::partitionEventsIndividual(inputFiles_, outputDirectory_,
                                                      {windowName_, windowStartTime_ + windowOffset_, windowWidth_},
                                                      windowSlices_, windowDelta_);
            else
                Processors::partitionEventsSummed(inputFiles_, outputDirectory_,
                                                  {windowName_, windowStartTime_ + windowOffset_, windowWidth_}, windowSlices_,
                                                  windowDelta_);
            break;
        default:
            throw(std::runtime_error("Unhandled processing mode.\n"));
    }

    return 0;
}
