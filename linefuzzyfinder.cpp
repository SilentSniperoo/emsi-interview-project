#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

class WordSet {
public:
    WordSet(const std::string& wordSetLine) : mLine(wordSetLine) {
        // We probably don't care about the casing
        std::for_each(mLine.begin(), mLine.end(), [](char& c) {
            c = static_cast<char>(std::tolower(c));
        });

        // Only some characters break words
        auto doesBreakWord = [](char c){
            return std::string(" (),.!:;\"“‘’”—").find(c) != std::string::npos;
        };

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

        // Could add checks for ordering and for similar words

        // Ordering could help with "he is" vs "is he"
        // Yet, ordering may pollute the metrics since gaps are expected
        // Could make better by weighting stronger with rarer words

        // Also, similar words may help with "make" vs "making"
        // Yet, similar words would cause serious problems with short words
        // Could make better by weighting stronger with longest common sequence

        // Weight and then scale down to the output range
        return (2 * words + runes) / 3;
    }

private:
    // Returns [-1, 1] where -1 is no words found and 1 is all words found and
    // with the exact number of appearances in both sets
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
        std::cerr << "Best score: " << bestScore << std::endl;
        return bestLine;
    }

private:
    // Only need to search the lines that aren't empty
    // Keep the original line numbers though, so we can return the correct line
    std::vector<std::pair<size_t, WordSet>> mNonEmtpyLines;
};

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

int main(int argc, char** argv) {
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
