#pragma once

#include <H5Cpp.h>
#include <gsl/gsl_histogram.h>
#include <map>
#include <string>
#include <vector>

class NeXuSFile
{
    public:
    NeXuSFile(std::string filename = "", bool printInfo = false);
    void operator=(NeXuSFile &source);
    NeXuSFile(const NeXuSFile &source);
    NeXuSFile(NeXuSFile &&source);
    ~NeXuSFile();
    // Clear all data and arrays
    void clear();
    // Copy data from specified source
    void copy(const NeXuSFile &source, bool deepCopyHistograms = true);

    /*
     * I/O
     */
    private:
    // Filename
    std::string filename_;

    private:
    // Return handle and (simple) dimension for named leaf dataset
    static std::pair<H5::DataSet, long int> find1DDataset(H5::H5File file, H5std_string terminal, H5std_string datasetName);

    public:
    // Return filename
    std::string filename() const;
    // Load basic information from the NeXuS file
    void loadBasicData(bool printInfo = false);
    // Template a new NeXusFile from that specified
    static void templateTo(std::string sourceFilename, std::string newFilename);
    // Prepare spectra storage, including loading TOF boundaries etc.
    void prepareSpectraSpace(bool printInfo = false);
    // Load in monitor histograms
    void loadMonitorCounts();
    // Load event data
    void loadEventData();
    // Load detector counts from the file
    void loadDetectorCounts();
    // Save key modified data back to the file
    bool saveModifiedData();

    /*
     * Data
     */
    private:
    std::vector<int> detectorSpectrumIndices_;
    int nMonitorSpectra_{0};
    int nMonitorFrames_{0};
    int nDetectorFrames_{0};
    int nGoodFrames_{0};
    int startSinceEpoch_{0};
    int endSinceEpoch_{0};
    std::vector<int> eventIndices_;
    std::vector<double> eventTimes_;
    std::vector<int> eventsPerFrame_;
    std::vector<double> frameOffsets_;
    std::vector<double> tofBoundaries_;
    std::map<int, std::vector<int>> monitorCounts_;
    std::map<unsigned int, std::vector<int>> detectorCounts_;
    std::map<unsigned int, gsl_histogram *> detectorHistograms_;

    public:
    [[nodiscard]] int nGoodFrames() const;
    [[nodiscard]] int nMonitorFrames() const;
    [[nodiscard]] int nDetectorFrames() const;
    void incrementDetectorFrameCount(int delta = 1);
    [[nodiscard]] int startSinceEpoch() const;
    [[nodiscard]] int endSinceEpoch() const;
    [[nodiscard]] const std::vector<int> &eventIndices() const;
    [[nodiscard]] const std::vector<double> &eventTimes() const;
    [[nodiscard]] const std::vector<int> &eventsPerFrame() const;
    [[nodiscard]] const std::vector<double> &frameOffsets() const;
    [[nodiscard]] const std::vector<double> &tofBoundaries() const;
    [[nodiscard]] const int spectrumForDetector(int detectorId) const;
    [[nodiscard]] const std::map<int, std::vector<int>> &monitorCounts() const;
    [[nodiscard]] const std::map<unsigned int, std::vector<int>> &detectorCounts() const;
    std::map<unsigned int, gsl_histogram *> &detectorHistograms();

    /*
     * Manipulation
     */
    public:
    // Scale monitors by specified factor
    void scaleMonitors(double factor);
    // Scale detectors by specified factor
    void scaleDetectors(double factor);
};
