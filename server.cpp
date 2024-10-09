//server.cpp
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/capability.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

using namespace boost::asio;
using ip::tcp;

// Class definition for HTTP server
class HttpServer {
public:

    // Constructor to initialize the server on port 8080
    HttpServer() : acceptor_(io_, tcp::endpoint(tcp::v4(), 8080)) {
        //initalize the logger
        logger = spdlog::basic_logger_mt("http_server_logger", "logs/http_server_log.txt");
        logger->info("Server started on port 8080");
    }

    // Method to run the server and handle incoming connections
    void run() {
        while (true) {
            std::unique_ptr<tcp::socket> socket = std::make_unique<tcp::socket>(io_);
            acceptor_.accept(*socket);

            // Fork a new process for each incoming connection
            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                handleRequest(std::move(*socket));
                exit(0);
            } else if (pid < 0) {
                logger->error("Error forking process");
            } else {
                // Parent process
                close(socket->native_handle()); // Close the socket in the parent process
                waitpid(pid, NULL, WNOHANG); // Reap the child process
            }
        }
    }

private:
    // Define the io context and acceptor
    io_service io_;
    tcp::acceptor acceptor_;
    // Define the logger
    std::shared_ptr<spdlog::logger> logger;

    // Method to handle incoming HTTP requests
    void handleRequest(tcp::socket socket) {
        try {
            boost::system::error_code error;
            boost::asio::streambuf request;
            read_until(socket, request, "\r\n\r\n", error);

            if (!error) {
                std::istream request_stream(&request);
                std::string request_method;
                std::string request_path;
                std::string post_data;

                request_stream >> request_method >> request_path;

                if (request_method == "GET") {
                    // Handle GET requests for file retrieval
                    std::string file_path = request_path.substr(1);
                    handleGetRequest(socket, file_path);
                } else if (request_method == "POST") {
                    // Handle POST requests
                    read_post_data(request_stream, post_data);
                    handlePostRequest(socket, post_data);
                }
            } else {
                throw std::runtime_error("Error reading request: " + error.message());
            }
        } catch (const std::exception& e) {
            logger->error("Error handling request: {}", e.what());
        }
    }

    // Method to handle GET requests for file retrieval
    void handleGetRequest(tcp::socket& socket, const std::string& file_path) {
        try {
            logger->info("Handling GET request for file: {}", file_path);
            std::string file_content = readFile(file_path);
            std::string response_headers = generateResponseHeaders(file_path);
            write(socket, buffer(response_headers));
            write(socket, buffer(file_content));
        } catch (const std::exception& e) {
            logger->error("Error handling GET request: {}", e.what());
        }
    }

    // Method to read POST data from the request
    void read_post_data(std::istream& request_stream, std::string& post_data) {
        try {
            logger->info("Reading POST data: {}", post_data);
            std::stringstream ss;
            ss << request_stream.rdbuf();  // Read the entire request body into a stringstream
            std::string request_body = ss.str();

            // Find the position of the title and content data in the request body
            size_t title_pos = request_body.find("title=");
            size_t content_pos = request_body.find("content=");

            if (title_pos != std::string::npos && content_pos != std::string::npos) {
                // Extract the title and content values from the request body
                std::string title_value = extract_field_value(request_body, title_pos);
                std::string content_value = extract_field_value(request_body, content_pos);

                // Save the title and content values to the post_data string
                post_data = "Title: " + title_value + "\nContent: " + content_value;
            } else {
                throw std::runtime_error("Title or content not found in the request body");
            }
        } catch (const std::exception& e) {
            logger->error("Error reading post data: {}", e.what());
        }
    }

    // Method to extract field value from the request body
    std::string extract_field_value(const std::string& request_body, size_t field_pos) {
        logger->info("Extracting field value...");
        size_t value_start = request_body.find_first_of("=", field_pos) + 1;
        size_t value_end = request_body.find_first_of("&", value_start);
        if (value_end == std::string::npos) {
            value_end = request_body.size();
        }
        std::string value = request_body.substr(value_start, value_end - value_start);

        // URL decode the value
        value = url_decode(value);

        logger->info("Value: {}", value);

        return value;
    }

    // Method to decode URL-encoded values
    std::string url_decode(const std::string& str) {
        logger->info("Decoding url encoded values: {}", str);
        std::stringstream decoded;
        std::string::const_iterator it = str.begin();
        while (it != str.end()) {
            if (*it == '%') {
                if (it + 3 <= str.end()) {
                    int value = 0;
                    std::istringstream is(std::string(it + 1, it + 3));
                    is >> std::hex >> value;
                    decoded << static_cast<char>(value);
                    it += 3;
                } else {
                    decoded << *it;
                }
            } else if (*it == '+') {
                decoded << ' ';
            } else {
                decoded << *it;
            }
            it++;
        }
        logger->info("decoded: {}", decoded.str());
        return decoded.str();
    }

    // Method to generate response headers based on file type
    std::string generateResponseHeaders(const std::string& file_path) {
        logger->info("Generating response header for file: {}", file_path);
        std::string content_type = "text/plain"; // Default content type
        if (file_path.find(".html") != std::string::npos) {
            content_type = "text/html";
        } else if (file_path.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (file_path.find(".js") != std::string::npos) {
            content_type = "application/javascript";
        }

        std::ostringstream headers;
        headers << "HTTP/1.1 200 OK\r\n";
        headers << "Content-Type: " << content_type << "\r\n";
        headers << "Connection: close\r\n\r\n";
        logger->info("headers: {}", headers.str());
        return headers.str();
    }

    // Method to read file content
    std::string readFile(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            logger->error("404 File Not Found");
            return "404 File Not Found";
        }

        std::ostringstream oss;
        oss << file.rdbuf();
        logger->info("oss: {}", oss.str());
        return oss.str();
    }

    // Method to handle POST requests and save data to a text file
    void handlePostRequest(tcp::socket& socket, const std::string& post_data) {
        logger->info("Handling POST request: {}", post_data);
        // Save post data to a text file
        std::string file_path = "output/post_data.txt";
        std::ofstream outfile(file_path);
        if (outfile.is_open()) {
            outfile << post_data;
            outfile.close();
            // Send HTTP response
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/plain\r\n";
            response << "Content-Length: " << post_data.size() << "\r\n";
            response << "Connection: close\r\n\r\n";
            response << "Data saved successfully\n";
            logger->info("response: {}", response.str());
            write(socket, buffer(response.str()));
        } else {
            // Handle file error
            std::ostringstream response;
            response << "HTTP/1.1 500 Internal Server Error\r\n";
            response << "Content-Type: text/plain\r\n";
            response << "Content-Length: " << 0 << "\r\n";
            response << "Connection: close\r\n\r\n";
            logger->info("response: {}", response.str());
            write(socket, buffer(response.str()));
        }
    }
};

// Main function to create an instance of HttpServer and start the server
int main() {
    HttpServer server;
    server.run();

    return 0;
}
