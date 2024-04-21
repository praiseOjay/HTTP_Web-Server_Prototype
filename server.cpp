//server.cpp
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
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
    // Define the logger
    std::shared_ptr<spdlog::logger> logger;

    // Constructor to initialize the server on port 8080
    HttpServer() : acceptor_(io_, tcp::endpoint(tcp::v4(), 8080)) {
        // Initialize the logger for logging server activities
        logger = spdlog::basic_logger_mt("http_server_logger", "logs/http_server_log.txt");
        logger->info("Server started on port 8080");
    }

    // Method to run the server and handle incoming connections
    void run() {
        while (true) {
            // Create a unique socket for each incoming connection
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
    io_service io_;
    tcp::acceptor acceptor_;

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
    std::string extract_field_value(const std::string& request_body, size_t pos) {
        // Find the end position of the field value
        size_t end_pos = request_body.find("&", pos);
        if (end_pos == std::string::npos) {
            end_pos = request_body.size();
        }
        // Extract the field value from the request body
        return request_body.substr(pos + 6, end_pos - pos - 6); // 6 is the length of "title=" or "content="
    }

    // Method to read file content based on the file path
    std::string readFile(const std::string& file_path) {
        std::ifstream file(file_path);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        } else {
            throw std::runtime_error("Failed to open file: " + file_path);
        }
    }

    // Method to generate response headers for the requested file
    std::string generateResponseHeaders(const std::string& file_path) {
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(file_path.size()) + "\r\n\r\n";
        return response;
    }

    // Method to handle POST requests
    void handlePostRequest(tcp::socket& socket, const std::string& post_data) {
        // Save post data to a file or process it accordingly
        // In this example, we are just logging the post data
        logger->info("Received POST data:\n{}", post_data);
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        write(socket, buffer(response));
    }
};

// Main function to create an instance of HttpServer and start the server
int main() {
    HttpServer server;
    server.run();

    return 0;
}
