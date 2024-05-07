#include "miner.hpp"

namespace Core {

    static bool redirectConsoleIO() {
        bool result = true;
        FILE* fp;

        auto handleIO = [&](const char* filename, const char* mode, int handleType) {
            if (GetStdHandle(handleType) != INVALID_HANDLE_VALUE) {
                if (freopen_s(&fp, filename, mode, stdin) != 0) {
                    result = false;
                } else {
                    setvbuf(stdin, nullptr, _IONBF, 0);
                }
            }
        };

        handleIO("CONIN$", "r", STD_INPUT_HANDLE);
        handleIO("CONOUT$", "w", STD_OUTPUT_HANDLE);
        handleIO("CONOUT$", "w", STD_ERROR_HANDLE);

        std::ios::sync_with_stdio(true);

        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::cin.clear();

        return result;
    }

    Miner::Miner(std::shared_ptr<HttpServer> server, std::vector<int>& peers, BlockChain& blockchain)
        : server(server), peers(peers), blockchain(blockchain) {
        initConsole();
        start(server, blockchain, peers);
        setUpPeer(std::move(server), peers, blockchain);
    }

    void Miner::initConsole() {
        CreateNewConsole(1024);
    }

    bool Miner::CreateNewConsole(int16_t length) {
        ReleaseConsole();
        if (AllocConsole()) {
            AdjustConsoleBuffer(length);
            return redirectConsoleIO();
        }
        return false;
    }

    void Miner::AdjustConsoleBuffer(int16_t minLength) {
        CONSOLE_SCREEN_BUFFER_INFO conInfo;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo)) {
            if (conInfo.dwSize.Y < minLength) {
                conInfo.dwSize.Y = minLength;
                SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);
            }
        }
    }

    [[nodiscard]]
    bool Miner::ReleaseConsole() {
        bool success = true;
        FILE* fp;

        auto resetIO = [&](const char* filename, const char* mode) {
            if (freopen_s(&fp, filename, mode, stdin) != 0) {
                success = false;
            }
            setvbuf(stdin, nullptr, _IONBF, 0);
        };

        resetIO("NUL:", "r");
        resetIO("NUL:", "w");
        if (!FreeConsole()) {
            success = false;
        }

        return success;
    }

    int Miner::getAvailablePort() {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> dist(3000, 4000);

        return dist(rng);
    }

    void Miner::writePort(unsigned int port) {
        std::ofstream file("file.txt", std::ios::out | std::ios::app);
        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open file.");
        }
        file << port << '\n';
    }

    std::vector<int> Miner::readPort(const char* path) {
        std::ifstream is(path);
        std::istream_iterator<int> start(is), end;
        return std::vector<int>(start, end);
    }

    void Miner::process_input(HWND handle, std::vector<int>& peers, BlockChain& blockchain) {
        std::string input;
        while (true) {
            std::cout << "Commands: /print, /add, [block name]\n";
            std::cin >> input;

            if (input == "/print") {
                blockchain.printBlocks();
            } else if (input == "/add") {
                addTransaction(blockchain, peers, handle);
            } else {
                bool found = false;
                for (int i = 0; i < blockchain.getNumOfBlocks(); ++i) {
                    if (blockchain.getByName(i) == input) {
                        blockchain.printBlock(i);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    spdlog::warn("Block with name '{}' not found.", input);
                }
            }
        }
    }

    void Miner::addTransaction(BlockChain& blockchain, std::vector<int>& peers, HWND handle) {
        std::string miner, in, out;
        float amount;

        spdlog::info("Enter miner's name:");
        std::cin >> miner;

        spdlog::info("Enter incoming transaction:");
        std::cin >> in;

        spdlog::info("Enter outgoing transaction:");
        std::cin >> out;

        spdlog::info("Enter transaction amount:");
        std::cin >> amount;

        std::string tx = fmt::format("Miner: {}, InTX: {}, OutTX: {}, Amount: {:.2f} WBT", miner, in, out, amount);

        std::vector<std::string> transaction{ tx };

        int difficulty;
        spdlog::info("Enter difficulty level:");
        std::cin >> difficulty;

        auto [blockHash, nonce] = Utils::findHash(difficulty, blockchain.getNumOfBlocks(), blockchain.getLatestBlockHash(), transaction);

        if ((blockchain.getNumOfBlocks() != 0) && ((blockchain.getNumOfBlocks() % 3) == 0)) {
            MessageBox(handle, "+0.25WBT", "Blockchain Message", MB_OK);
        }

        blockchain.addBlock(difficulty, blockchain.getNumOfBlocks(), Block::getTime(), 
                            blockchain.getLatestBlockHash(), blockHash, nonce, transaction);

        spdlog::info("Updating blockchain...");
        for (const auto& port : peers) {
            try {
                HttpClient client(fmt::format("localhost:{}", port));
                client.request("POST", "/updateLedger", blockchain.serialize());
            } catch (const std::exception& ex) {
                spdlog::warn("Failed to update peer at port {}: {}", port, ex.what());
            }
        }
    }

    int Miner::start(std::shared_ptr<HttpServer> server, BlockChain& blockchain, std::vector<int>& peers) {
        char user_input;
        std::cout << "Welcome to WinCoin.\nAre you creating a new blockchain ('y') or joining an existing one ('j')?\n";
        std::cin >> user_input;

        server->config.port = getAvailablePort();
        writePort(server->config.port);

        if (user_input == 'y') {
            blockchain.behave(BlockChain::Stage::GENESIS);
        } else if (user_input == 'j') {
            peers = readPort("file.txt");
            blockchain.behave(BlockChain::Stage::JOIN);

            nlohmann::json peer_info;
            peer_info["port"] = server->config.port;

            for (const auto& port : peers) {
                HttpClient client(fmt::format("localhost:{}", port));
                client.request("POST", "/peerpush", peer_info.dump());
            }

            updateBlockchainFromPeers(peers, blockchain);
        } else {
            spdlog::warn("Invalid input.");
            return 0;
        }

        return 1;
    }

    void Miner::updateBlockchainFromPeers(const std::vector<int>& peers, BlockChain& blockchain) {
        std::vector<std::string> blockchains;
        for (const auto& port : peers) {
            try {
                HttpClient client(fmt::format("localhost:{}", port));
                auto response = client.request("GET", "/current");
                blockchains.push_back(response->content.string());
            } catch (const std::exception& ex) {
                spdlog::warn("Error fetching blockchain from peer at port {}: {}", port, ex.what());
            }
        }

        if (blockchains.empty()) {
            spdlog::error("No blockchains found from peers.");
            return;
        }

        auto longest_blockchain = std::max_element(
            blockchains.begin(), 
            blockchains.end(), 
            [](const std::string& a, const std::string& b) {
                nlohmann::json json_a = nlohmann::json::parse(a);
                nlohmann::json json_b = nlohmann::json::parse(b);
                return json_a["length"].get<int>() < json_b["length"].get<int>();
            }
        );

        nlohmann::json json_blockchain = nlohmann::json::parse(*longest_blockchain);
        for (size_t i = 0; i < json_blockchain["length"].get<int>(); ++i) {
            auto block = json_blockchain["data"][i];
            std::vector<std::string> data = block["data"].get<std::vector<std::string>>();
            blockchain.addBlock(block["difficulty"], block["counter"], block["minedtime"], 
                                block["previousHash"], block["hash"], block["nonce"], data);
        }
    }
} 
