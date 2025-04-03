#include <iostream>
#include <string>
#include <queue>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <stdexcept>
#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>


using namespace std;
using namespace rapidjson;

bool debug = false;

// Updated service URL
const string SERVICE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";
const int MAX_THREADS = 8;

mutex visited_mutex; 
mutex next_level_mutex;
mutex cout_mutex;

// Function to HTTP ecnode parts of URLs. for instance, replace spaces with '%20' for URLs
string url_encode(CURL* curl, string input) {
    char* out = curl_easy_escape(curl, input.c_str(), input.size());
    string s = out;
    curl_free(out);
    return s;
}

// Callback function for writing response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Function to fetch neighbors using libcurl
string fetch_neighbors(CURL* curl, const string& node) {
    string url = SERVICE_URL + url_encode(curl, node);
    string response;

    if (debug) {
        lock_guard<mutex> cout_lock(cout_mutex);
        cout << "Sending request to: " << url << endl;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Set a User-Agent header to avoid potential blocking by the server
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: C++-Client/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        lock_guard<mutex> cout_lock(cout_mutex);
        cerr << "CURL error: " << curl_easy_strerror(res) << endl;
    }

    // Cleanup
    curl_slist_free_all(headers);

    if (debug) {
        lock_guard<mutex> cout_lock(cout_mutex);
        cout << "Response received: " << response << endl; // Debug log
    }

    return (res == CURLE_OK) ? response : "{}";
}

// Function to parse JSON and extract neighbors
vector<string> get_neighbors(const string& json_str) {
    vector<string> neighbors;
    Document doc;
    doc.Parse(json_str.c_str());
    
    if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
        for (const auto& neighbor : doc["neighbors"].GetArray())
            neighbors.push_back(neighbor.GetString());
    }
    return neighbors;
}

// Worker function for parallel processing of nodes at a level
void process_nodes(
    CURL* curl, 
    const vector<string>& nodes_to_process, 
    unordered_set<string>& visited, 
    vector<string>& next_level,
    int start_idx, 
    int end_idx
) {
    // Create a local CURL handle for this thread
    CURL* local_curl = curl_easy_init();
    if (!local_curl) {
        lock_guard<mutex> cout_lock(cout_mutex);
        cerr << "Failed to initialize CURL in thread" << endl;
        return;
    }

    vector<string> local_next_level;

    for (int i = start_idx; i < end_idx; i++) {
        const string& node = nodes_to_process[i];
        
        if (debug) {
            lock_guard<mutex> cout_lock(cout_mutex);
            cout << "Thread " << this_thread::get_id() << " processing node: " << node << endl;
        }

        try {
            string response = fetch_neighbors(local_curl, node);
            vector<string> neighbors = get_neighbors(response);

            for (const auto& neighbor : neighbors) {
                // Check if we've already visited this node
                bool already_visited;
                {
                    lock_guard<mutex> visited_lock(visited_mutex);
                    already_visited = (visited.count(neighbor) > 0);
                }

                if (!already_visited) {
                    // Mark as visited and add to next level
                    {
                        lock_guard<mutex> visited_lock(visited_mutex);
                        if (visited.count(neighbor)) continue; // Double-check
                        visited.insert(neighbor);
                    }
                    local_next_level.push_back(neighbor);
                }
            }
        } catch (const exception& e) {
            lock_guard<mutex> cout_lock(cout_mutex);
            cerr << "Error while processing node " << node << ": " << e.what() << endl;
        }
    }

    // Merge local results with global next level
    if (!local_next_level.empty()) {
        lock_guard<mutex> next_lock(next_level_mutex);
        next_level.insert(next_level.end(), local_next_level.begin(), local_next_level.end());
    }

    curl_easy_cleanup(local_curl);
}

// Parallel BFS Traversal Function
vector<vector<string>> bfs(const string& start, int depth) {
    vector<vector<string>> levels;
    unordered_set<string> visited;

    levels.push_back({start});
    visited.insert(start);

    CURL* main_curl = curl_easy_init();
    if (!main_curl) {
        cerr << "Failed to initialize CURL" << endl;
        return levels;
    }

    for (int d = 0; d < depth; d++) {
        if (debug) {
            lock_guard<mutex> cout_lock(cout_mutex);
            cout << "Starting level " << d << " with " << levels[d].size() << " nodes" << endl;
        }

        levels.push_back({});
        vector<string>& current_level = levels[d];
        vector<string>& next_level = levels[d+1];

        if (current_level.empty()) {
            break;
        }

        int num_nodes = current_level.size();
        int num_threads = min(MAX_THREADS, num_nodes);
        int nodes_per_thread = num_nodes / num_threads;
        int remaining_nodes = num_nodes % num_threads;

        if (debug) {
            lock_guard<mutex> cout_lock(cout_mutex);
            cout << "Using " << num_threads << " threads for this level" << endl;
        }

        vector<thread> threads;

        int start_idx = 0;
        for (int i = 0; i < num_threads; i++) {
            int end_idx = start_idx + nodes_per_thread + (i < remaining_nodes ? 1 : 0);
            
            if (start_idx < end_idx) {
                threads.emplace_back(
                    process_nodes, 
                    main_curl,
                    ref(current_level),
                    ref(visited),
                    ref(next_level),
                    start_idx,
                    end_idx
                );
            }
            
            start_idx = end_idx;
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    curl_easy_cleanup(main_curl);
    return levels;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <node_name> <depth>\n";
        return 1;
    }

    string start_node = argv[1];  // example "Tom%20Hanks"
    int depth;
    try {
        depth = stoi(argv[2]);
    } catch (const exception& e) {
        cerr << "Error: Depth must be an integer.\n";
        return 1;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return -1;
    }

    const auto start{chrono::steady_clock::now()};
    for (const auto& n :  bfs(start_node, depth)) {
        for (const auto& node : n) {
            cout << "- " << node << "\n";
        }
        cout << "\n# of nodes in this level: " << n.size() << "\n\n";
    }
    
    const auto finish{chrono::steady_clock::now()};
    const chrono::duration<double> elapsed_seconds{finish - start};
    cout << "------------------------------------------------------\n";
    cout << "Time to crawl: " << elapsed_seconds.count() << "s\n";
    cout << "------------------------------------------------------\n\n";
    
    curl_easy_cleanup(curl);
    
    return 0;
}

