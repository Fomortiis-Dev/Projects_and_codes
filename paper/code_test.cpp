#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <sys/resource.h>

void printMemoryUsage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "Memory usage: " << usage.ru_maxrss << " KB" << std::endl;
    } else {
        std::cerr << "Error getting memory usage." << std::endl;
    }
}

// Define minUtility globally or in main
double minUtility = 65; 

// Structure to represent an item with internal utility
struct Item {
    int id;
    int internalUtility;
};

// Structure to represent a sequence
struct Sequence {
    std::vector<Item> items;
    int sUtility;  // Sequence utility directly given in the dataset
};

// Structure for a node in the utility tree
struct TreeNode {
    std::string label;
    int SID;       // Sequence ID
    int UT;        // Utility
    int PUT;       // Previous Utility
    int RUT;       // Remaining Utility
    TreeNode* left;
    TreeNode* right;
    std::vector<TreeNode*> nodeLink;
};

// Function to parse a sequence from a line in the file
Sequence parseSequence(const std::string& line) {
    Sequence sequence;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        if (token == "-1" || token == "-2") continue;

        if (token.find("SUtility:") != std::string::npos) {
            sequence.sUtility = std::stoi(token.substr(9));
        } else {
            size_t openBracket = token.find('[');
            size_t closeBracket = token.find(']');
            Item item;
            item.id = std::stoi(token.substr(0, openBracket));
            item.internalUtility = std::stoi(token.substr(openBracket + 1, closeBracket - openBracket - 1));
            sequence.items.push_back(item);
        }
    }
    return sequence;
}

// Function to calculate the utility, previous utility, and remaining utility for an item
void calculateUtilities(TreeNode* node, const std::vector<Item>& items, int sUtility) {
    node->UT = sUtility;
    node->PUT = 0; // Assuming previous utility is initially 0
    node->RUT = sUtility; // Initially, the entire utility is considered remaining

    // Adjust PUT and RUT as needed
    for (const auto& item : items) {
        if (std::stoi(node->label) == item.id) break;
        node->PUT += item.internalUtility;
        node->RUT -= item.internalUtility;
    }
}

// Function to add transactions to the utility tree
void addTransaction(TreeNode* root, const std::vector<Item>& transaction, int sUtility, int SID) {
    if (transaction.empty()) return;
    std::string label = std::to_string(transaction.front().id);
    TreeNode* child = nullptr;

    // Check if child with the same label exists
    for (auto& node : root->nodeLink) {
        if (node->label == label) {
            child = node;
            break;
        }
    }

    if (!child) {
        // Create a new node
        child = new TreeNode{ label, SID, 0, 0, 0, nullptr, nullptr, {} };
        root->nodeLink.push_back(child);
    }

    // Calculate utilities for the current item
    calculateUtilities(child, transaction, sUtility);

    // Prune based on minUtility
    if (child->UT < minUtility) {
        return; // Skip adding this node if its utility is below minUtility
    }

    // Recursively add the remaining transaction
    std::vector<Item> remainingTransaction(transaction.begin() + 1, transaction.end());
    addTransaction(child, remainingTransaction, sUtility, SID);
}

// Function to perform tree growth and generate rules
void treeGrowth(TreeNode* tree, const std::string& Y, bool isLeft, std::ofstream& outputFile) {
    if (!tree) return;

    if (tree->nodeLink.size() == 1) {
        // Handle single path rule generation logic
        TreeNode* currentNode = tree->nodeLink.front();
        std::string path = Y;
        int cumulativeUtility = 0;
        while (currentNode) {
            path += " -> " + currentNode->label;
            cumulativeUtility += currentNode->UT;
            if (!currentNode->nodeLink.empty()) {
                currentNode = currentNode->nodeLink.front();
            } else {
                currentNode = nullptr;
            }
        }
        outputFile << "Single Path Rule: " << path
                   << " [Cumulative Utility: " << cumulativeUtility << "]\n";
        std::cout << "Single Path Rule: " << path
                  << " [Cumulative Utility: " << cumulativeUtility << "]\n";
    } else {
        for (auto& node : tree->nodeLink) {
            if (node->UT >= minUtility) { // Check against minUtility
                // Print or save the rule
                outputFile << "Rule: " << Y << " -> " << node->label
                           << " [SID: " << node->SID << ", Utility: " << node->UT
                           << ", PUT: " << node->PUT << ", RUT: " << node->RUT << "]\n";
                std::cout << "Rule: " << Y << " -> " << node->label
                          << " [SID: " << node->SID << ", Utility: " << node->UT
                          << ", PUT: " << node->PUT << ", RUT: " << node->RUT << "]\n";
                // Grow the tree
                treeGrowth(node, Y + node->label, isLeft, outputFile);
            }
        }
    }
}

int main() {
    std::ifstream inputFile("test.txt");
    if (!inputFile) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    std::ofstream outputFile("output.txt");
    if (!outputFile) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    std::vector<Sequence> sequences;
    std::string line;
    while (std::getline(inputFile, line)) {
        sequences.push_back(parseSequence(line));
    }
    inputFile.close();

    // Measure the start time
    auto start = std::chrono::high_resolution_clock::now();

    // Build the utility tree
    TreeNode* root = new TreeNode{ "root", 0, 0, 0, 0, nullptr, nullptr, {} };
    for (size_t i = 0; i < sequences.size(); ++i) {
        addTransaction(root, sequences[i].items, sequences[i].sUtility, i + 1);
    }

    // Perform tree growth and generate rules
    treeGrowth(root, "", true, outputFile);

    // Measure the end time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Log the execution time and min utility to a separate file
    std::ofstream logFile("execution_log.txt", std::ios::app); // Append mode
    if (logFile) {
        logFile << "Min Utility: " << minUtility << ", Execution Time: " << elapsed.count() << " seconds\n";
        logFile.close();
    }

    std::cout << "High-Utility Partial Ordered Sequential Rules Mining completed successfully!" << std::endl;
    printMemoryUsage(); // Call this function to check memory usage
    std::cout << "Execution Time: " << elapsed.count() << " seconds" << std::endl;
    outputFile.close();
    return 0;
}
