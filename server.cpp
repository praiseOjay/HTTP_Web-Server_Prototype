//server.cpp
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;

class HttpServer {
public:
    HttpServer() : acceptor_(io_, tcp::endpoint(tcp::v4(), 8080)) {
        std::cout << "Server started on port 8080" << std::endl;
    }

    void run() {
        while (true) {
            tcp::socket socket(io_);
            acceptor_.accept(socket);

            handleRequest(socket);
        }
    }

private:
    io_service io_;
    tcp::acceptor acceptor_;

    void handleRequest(tcp::socket& socket) {
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
        }
    }

    void handleGetRequest(tcp::socket& socket, const std::string& file_path) {
    std::string file_content = readFile(file_path);
    std::string response_headers = generateResponseHeaders(file_path);
    write(socket, buffer(response_headers));
    write(socket, buffer(file_content));
    std::cout << response_headers << std::endl;
    }

    void read_post_data(std::istream& request_stream, std::string& post_data) {
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
    }
}


std::string extract_field_value(const std::string& request_body, size_t field_pos) {
    size_t value_start = request_body.find_first_of("=", field_pos) + 1;
    size_t value_end = request_body.find_first_of("&", value_start);
    if (value_end == std::string::npos) {
        value_end = request_body.size();
    }
    std::string value = request_body.substr(value_start, value_end - value_start);
    
    // URL decode the value
    value = url_decode(value);
    
    return value;
}

std::string url_decode(const std::string& str) {
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
    return decoded.str();
}

    std::string generateResponseHeaders(const std::string& file_path) {
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

    return headers.str();
    }

    std::string readFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return "File Not Found";
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
    }

    void handlePostRequest(tcp::socket& socket, const std::string& post_data) {
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
            write(socket, buffer(response.str()));
        } else {
            // Handle file error
            std::ostringstream response;
            response << "HTTP/1.1 500 Internal Server Error\r\n";
            response << "Content-Type: text/plain\r\n";
            response << "Content-Length: " << 0 << "\r\n";
            response << "Connection: close\r\n\r\n";
            write(socket, buffer(response.str()));
        }
    }
};

int main() {
    HttpServer server;
    server.run();

    return 0;
}
