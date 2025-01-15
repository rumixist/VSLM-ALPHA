#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <iterator>
#include <random>

using namespace std;

typedef pair<string, string> Pair;
struct pair_hash {
    template <class T1, class T2>
    size_t operator()(const pair<T1, T2>& p) const {
        auto h1 = hash<T1>{}(p.first);
        auto h2 = hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

typedef unordered_map<Pair, unordered_map<string, int>, pair_hash> NGramMap;

// String'i küçük harfe çevirir
string toLower(const string& str) {
    string result;
    transform(str.begin(), str.end(), back_inserter(result), ::tolower);
    return result;
}

// Kelimeleri metinden çıkarır
vector<string> tokenize(const string& text) {
    istringstream iss(text);
    return {istream_iterator<string>{iss}, istream_iterator<string>{}};
}

// Noktalama işaretlerini kaldırır
string removePunctuation(const string& word) {
    string result;
    copy_if(word.begin(), word.end(), back_inserter(result), [](char c) { return isalnum(c) || isspace(c); });
    return result;
}

// N-gram sayımlarını yapar ve dosyaya kaydeder
void countNGrams(const string& filename, NGramMap& ngramMap) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        return;
    }

    string line;
    vector<string> words;
    while (getline(infile, line)) {
        line = toLower(line);
        vector<string> lineWords = tokenize(line);
        for (auto& word : lineWords) {
            word = removePunctuation(word);
        }
        words.insert(words.end(), lineWords.begin(), lineWords.end());
    }

    for (size_t i = 0; i + 2 < words.size(); ++i) {
        Pair key = make_pair(words[i], words[i + 1]);
        string nextWord = words[i + 2];
        ngramMap[key][nextWord]++;
    }

    infile.close();
}

// N-gram sayımlarını dosyaya yazar
void writeNGramsToFile(const string& filename, const NGramMap& ngramMap) {
    ofstream outfile(filename, ios::trunc); // Truncate the file to overwrite it
    if (!outfile.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        return;
    }

    for (const auto& entry : ngramMap) {
        outfile << entry.first.first << " " << entry.first.second;
        const auto& nextWords = entry.second;
        for (const auto& nextWord : nextWords) {
            outfile << " " << nextWord.first << " " << nextWord.second;
        }
        outfile << endl;
    }

    outfile.close();
}

// Dosyadan n-gram verilerini okuyarak ngramMap'i doldurur
void loadNGramsFromFile(const string& filename, NGramMap& ngramMap) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        istringstream iss(line);
        string word1, word2;
        iss >> word1 >> word2;
        Pair key = make_pair(word1, word2);

        string nextWord;
        int count;
        while (iss >> nextWord >> count) {
            ngramMap[key][nextWord] = count;
        }
    }

    infile.close();
}

// Verilen ağırlıklara göre rastgele bir kelime seçer
string getRandomNextWord(const unordered_map<string, int>& nextWords) {
    // Toplam ağırlık
    int totalWeight = 0;
    for (const auto& nextWord : nextWords) {
        totalWeight += nextWord.second;
    }

    // Rastgele bir değer seç
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, totalWeight - 1);
    int randomWeight = dis(gen);

    // Rastgele değere karşılık gelen kelimeyi bul
    for (const auto& nextWord : nextWords) {
        if (randomWeight < nextWord.second) {
            return nextWord.first;
        }
        randomWeight -= nextWord.second;
    }

    return "";  // Default return, aslında buraya ulaşmamalı
}

// İki kelimeye göre tahmin yapar
void generateSentence(const NGramMap& ngramMap, const string& first, const string& second, const int& wordCount) {
    Pair key = make_pair(first, second);
    if (ngramMap.find(key) == ngramMap.end()) {
        cout << "No predictions available for given words." << endl;
        return;
    }

    string currentFirst = first;
    string currentSecond = second;
    cout << first << " " << second;

    for (int i = 0; i < wordCount; ++i) {
        key = make_pair(currentFirst, currentSecond);
        if (ngramMap.find(key) == ngramMap.end()) {
            break;
        }

        const auto& nextWords = ngramMap.at(key);
        string nextWord = getRandomNextWord(nextWords);
        cout << " " << nextWord;
        currentFirst = currentSecond;
        currentSecond = nextWord;
    }
    cout << endl;
}

int main(int argc, char* argv[]) {
    NGramMap ngramMap;

    if (argc == 3 && string(argv[1]) == "count") {
        string inputFile = argv[2];
        countNGrams(inputFile, ngramMap);
        writeNGramsToFile("output.txt", ngramMap);
    } else if (argc == 5 && string(argv[1]) == "generate") {
        loadNGramsFromFile("output.txt", ngramMap);
        string word1 = argv[3];
        string word2 = argv[4];
        int wordCount = stoi(argv[2]);
        generateSentence(ngramMap, word1, word2, wordCount);
    } else {
        cerr << "Usage: " << argv[0] << " <count|generate> <trainfile.txt> | <wordCount> <word1> <word2>" << endl;
    }

    return 0;
}
