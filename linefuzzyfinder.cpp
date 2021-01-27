#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

#define DEFAULT_PATH "./lepanto.txt"

void printUsage();

bool readAllLines(const std::string& path, std::vector<std::string>& lines);

int driverMain(int argc, char** argv);

class WordSet {
public:
    WordSet(const std::string& wordSetLine) : mLine(wordSetLine) {
        // We don't care about the casing
        std::for_each(mLine.begin(), mLine.end(), [](char& c) {
            c = static_cast<char>(std::tolower(c));
        });

        // We only need one word break at a time since we only care about words
        // If we remove it now, we won't have to later over and over again
        // That will make comparisons where we only care about words easier
        bool isBreakingWord = true;
        for (size_t begin = 0, end = 0; end < mLine.size(); ++end) {
            if (!doesBreakWord(mLine[end])) {
                mLine[begin++] = mLine[end];
                isBreakingWord = false;
            }
            else if (!isBreakingWord) {
                mLine[begin++] = ' ';
                isBreakingWord = true;
            }
        }

        for (size_t begin = 0, end = 0; end <= mLine.size(); ++end) {
            // Count each word's appearances in the list of words
            const bool unended = end < mLine.size();
            const bool breaksWord = unended && doesBreakWord(mLine[end]);
            if (end > begin && (breaksWord || end >= mLine.size())) {
                std::string word(mLine.substr(begin, end - begin));
                if (auto iter = mWords.find(word); iter != mWords.end()) {
                    ++iter->second;
                }
                else {
                    mWords.emplace(word, 1);
                }
                begin = end + 1;
            }
            // Count each character's appearances in the list of words
            if (unended) {
                if (auto iter = mRunes.find(mLine[end]); iter != mRunes.end()) {
                    ++iter->second;
                }
                else {
                    mRunes.emplace(mLine[end], 1);
                }
            }
        }
    }

    // Returns [-1, 1] where 0 is fully dissimilar and 1 is a perfect match
    // Prioritizes making sure the other word set is contained in this word set
    double measureContainment(const WordSet& other) const {
        // Early out for perfect matches
        if (mLine == other.mLine) {
            return 1.0;
        }

        // Without taking into account ordering, search for words and characters
        // Gives a general sense of the match that is more kind to human error
        const double words = measureContainment(mWords, other.mWords);
        const double runes = measureContainment(mRunes, other.mRunes);

        // Find the longest common sequence of the line and scale to range
        const double fullShared = measureShared(mLine, other.mLine) * 2 - 1;
        // Find the average longest common sequence of words and scale to range
        const double wordShared = measureShared(mWords, other.mWords) * 2 - 1;

        // Weight and then scale down to the output range
        return (words + runes + fullShared + wordShared) / 4;
    }

private:
    // Returns [-1, 1] where -1 is no similar words found and 1 is all words
    // found and with the exact number of appearances in both sets
    template<typename T>
    static double measureContainment(
        const std::unordered_map<T, size_t>& t1,
        const std::unordered_map<T, size_t>& t2) {
        // All the same items the same number of times in a different order is a
        // very good match
        double itemsFound = 1, itemsPossible = 1;
        if (t1 != t2) {
            // See how well we can find items in our item set
            for (auto&& item : t2) {
                if (auto iter = t1.find(item.first); iter != t1.end()) {
                    // An item we searched for and found is good
                    // Matching the appearance count is best
                    // Under and overshooting are not as good, but still OK
                    if (item.second <= iter->second) {
                        itemsFound += static_cast<double>(item.second);
                        itemsPossible += static_cast<double>(iter->second);
                    }
                    else {
                        // Reverse the ratio to incentivize not overshooting
                        itemsFound += static_cast<double>(iter->second);
                        itemsPossible += static_cast<double>(item.second);
                    }
                }
                else {
                    // An item we searched for, but could not find is very bad
                    itemsFound -= static_cast<double>(item.second);
                    itemsPossible += static_cast<double>(item.second);
                }
            }
        }
        return itemsFound / itemsPossible;
    }

    // Returns true if the character acts as a separator for words
    static bool doesBreakWord(char c) {
        return std::string(" (),.!:;\"“‘’”—").find(c) != std::string::npos;
    }

    // Returns the longest string of consecutive matching characters
    static size_t countShared(const std::string& a, const std::string& b) {
        size_t longest = 0;
        for (size_t i = 0; i < a.size(); ++i) {
            for (size_t j = 0; j < b.size(); ++j) {
                size_t length = 0;
                while (
                    ((i + length) < a.size()) &&
                    ((j + length) < b.size()) &&
                    (a[i + length] == b[j + length])) {
                    ++length;
                }
                longest = length > longest ? length : longest;
            }
        }
        return longest;
    }

    // Returns the longest string of consecutive matching characters divided by
    // the average length of the strings (using the average length penalizes
    // having different lengths, while using the shorter string's length, the
    // longest possible sequence given the lengths, would treat one being a
    // substring of the other as a perfect match)
    static double measureShared(const std::string& a, const std::string& b) {
        const double sizeSum = static_cast<double>(a.size() + b.size());
        return static_cast<double>(countShared(a, b) * 2) / sizeSum;
    }

    // Searches each of the items of a for the longest shared sequence with b
    // Returns the longest string of consecutive matching characters of items in
    // a with b and divided by the average length of the strings (using the
    // average length penalizes having different lengths, while using the
    // shorter string's length, the longest possible sequence given the lengths,
    // would treat one being a substring of the other as a perfect match)
    static double measureShared(
        const std::unordered_map<std::string, size_t>& a,
        const std::string& b) {
        // We only care about the best match
        double bestShared = 0;
        for (auto&& pair : a) {
            const double shared = measureShared(pair.first, b);
            bestShared = shared > bestShared ? shared : bestShared;
        }
        return bestShared;
    }

    // For each item of b, searches each of the items of a for the longest
    // shared sequence with the item of b
    // Returns the average of the lengths of the shared sequences from each
    // found best match for each item in b
    static double measureShared(
        const std::unordered_map<std::string, size_t>& a,
        const std::unordered_map<std::string, size_t>& b) {
        // We need to average the results
        double measurementSum = 0;
        for (auto&& pair : b) {
            measurementSum += measureShared(a, pair.first);
        }
        return measurementSum / static_cast<double>(b.size());
    }

    std::string mLine;
    std::unordered_map<std::string, size_t> mWords;
    std::unordered_map<char, size_t> mRunes;
};

class Document {
public:
    Document(const std::vector<std::string>& documentLines) {
        for (size_t i = 0; i < documentLines.size(); ++i) {
            auto&& line = documentLines[i];
            if (line.size() > 0) {
                mNonEmtpyLines.emplace_back(i, line);
            }
        }
    }

    // Returns the index of the line that best matches the words given
    size_t fuzzyFind(const WordSet& wordSet) const {
        size_t bestLine = 0;
        double bestScore = -1.0;
        for (auto&& line : mNonEmtpyLines) {
            const double score = line.second.measureContainment(wordSet);
            // If we get a perfect match, stop the search immediately
            if (score == 1.0) {
                return line.first;
            }
            if (score > bestScore) {
                bestLine = line.first;
                bestScore = score;
            }
        }
        return bestLine;
    }

private:
    // Only need to search the lines that aren't empty
    // Keep the original line numbers though, so we can return the correct line
    std::vector<std::pair<size_t, WordSet>> mNonEmtpyLines;
};

int main(int argc, char** argv) {
    // If no arguments, use a defalt text file location and wait for input
    if (argc <= 1) {
        // Prompt now so the display is not delayed by the document loading
        std::cout << '>';
        std::cout.flush();
        // The document should be loaded quite quickly while waiting for input
        std::vector<std::string> defaultDocumentLines;
        if (!readAllLines(DEFAULT_PATH, defaultDocumentLines)) {
            std::cout << "Could not open default source file: " << DEFAULT_PATH
                << std::endl;
            printUsage();
            return 1;
        }
        Document document(defaultDocumentLines);
        // Fetch a line of input for the word set
        std::string wordSetLine;
        std::getline(std::cin, wordSetLine);
        // Search the loaded document for the word set and output the match
        size_t documentLineIndex = document.fuzzyFind(WordSet(wordSetLine));
        std::cout << defaultDocumentLines[documentLineIndex] << std::endl;
        return 0;
    }
    // Otherwise, expect the format matching the CLI driver usage
    driverMain(argc, argv);
}

void printUsage() {
    std::cout <<
        "linefuzzyfinder\n"
        "\n"
        "NAME\n"
        "\tlinefuzzyfinder - finds a line similar to input words\n"
        "\n"
        "SYNOPSIS\n"
        "\tUsage: linefuzzyfinder [-d documentFilepath] [-i wordSetFilepath]\n"
        "\tUsage: linefuzzyfinder [-d documentFilepath] [-c ...]\n"
        "\n"
        "DESCRIPTION\n"
        "\tlinefuzzyfinder is a pattern matcher that finds the most similar "
        "line of text from a document to a set of words. The set of words can "
        "be provided as a file with each set on its own line, or as quoted sets"
        " of words on the command line.\n"
        "\n"
        "EXAMPLES\n"
        "\tlinefuzzyfinder -d ./lepanto.txt -i ./testInputs.txt\n"
        "\t\tFinds the closest matching lines in \"./lepanto.txt\" to each set "
        "of words on each line of \"./testInputs.txt\".\n"
        "\n"
        "\tlinefuzzyfinder -d ./lepanto.txt -c \"his head a flag\" \"test word "
        "set two\" \"set three\"\n"
        "\t\tFinds the closest matching lines in \"./lepanto.txt\" to each set "
        "of words given in quotes.\n";
}

bool readAllLines(const std::string& path, std::vector<std::string>& lines) {
    std::ifstream stream(path);
    if (stream.is_open()) {
        std::string line;
        while (std::getline(stream, line)) {
            lines.emplace_back(std::move(line));
        };
        return lines.size() > 0;
    }
    return false;
}

#define DOCUMENT_FLAG_INDEX 1
#define DOCUMENT_ARGUMENT_INDEX 2
#define WORD_SET_FLAG_INDEX 3
#define WORD_SET_ARGUMENT_INDEX 4
#define MINIMUM_ARGUMENT_COUNT 5

int driverMain(int argc, char** argv) {
    // Format validation (program name, -d, document, -i/-c, word set...)
    if (argc < MINIMUM_ARGUMENT_COUNT) {
        std::cout << "Expected at least " << MINIMUM_ARGUMENT_COUNT
            << " arguments, but received " << argc << std::endl;
        printUsage();
        return 1;
    }
    if (argv[DOCUMENT_FLAG_INDEX] != std::string("-d")) {
        std::cout << "Missing \"-d\" flag." << std::endl;
        printUsage();
        return 1;
    }
    const std::string wordSetFlag(argv[WORD_SET_FLAG_INDEX]);
    if (wordSetFlag != "-i" && wordSetFlag != "-c") {
        std::cout << "Missing \"-i\" or \"-c\" flag." << std::endl;
        printUsage();
        return 1;
    }

    // Parameter validation (files present and input is readable)
    std::vector<std::string> documentLines;
    if (!readAllLines(argv[DOCUMENT_ARGUMENT_INDEX], documentLines)) {
        std::cout << "Could not open source file" << std::endl;
        printUsage();
        return 1;
    }
    std::vector<std::string> wordSetLines;
    if (wordSetFlag == std::string("-i")) {
        // Get input word sets from a file
        if (!readAllLines(argv[WORD_SET_ARGUMENT_INDEX], wordSetLines)) {
            std::cout << "Could not open input word set file" << std::endl;
            printUsage();
            return 1;
        }
    }
    else {
        // Get input word sets from the command line
        for (int i = WORD_SET_ARGUMENT_INDEX; i < argc; ++i) {
            wordSetLines.push_back(argv[i]);
        }
    }

    // Preprocess the data set once so we don't have to do it on each search
    Document document(documentLines);

    // Process the data and input
    for (auto&& wordSetLine : wordSetLines) {
        std::cout << "Searching for word set: \"" << wordSetLine << "\"\n";
        size_t documentLineIndex = document.fuzzyFind(WordSet(wordSetLine));
        std::cout << "Found line " << documentLineIndex << ": \""
            << documentLines[documentLineIndex] << "\"" << std::endl;
    }
    return 0;
}
