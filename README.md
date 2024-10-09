# Secure HTTP Web Server

A prototype secure web server implemented in C++ that handles HTTP GET and POST requests, reads and sends files, and saves post data to a text file. This server focuses on implementing various security measures to protect against common vulnerabilities and attacks.

## Features

- Handles GET and POST requests
- Reads and sends files
- Saves POST data to a text file
- Implements multiple security measures

## Security Measures

1. **Object-Oriented Programming (OOP)**
   - Enhances code organization and maintainability
   - Improves modularity and extensibility

2. **Safe Resource Handling**
   - Uses smart pointers (std::unique_ptr and std::shared_ptr)
   - Ensures proper resource deallocation and prevents memory leaks

3. **Error Handling**
   - Implements exception handling with try-catch blocks
   - Prevents server crashes and provides meaningful error feedback

4. **URL Decoding**
   - Safely handles untrusted inputs
   - Prevents injection of harmful characters in POST requests

5. **Multi-Process Architecture**
   - Uses forking to handle multiple incoming connections concurrently
   - Isolates each connection for improved security

6. **Logging**
   - Utilizes the spdlog library for comprehensive logging
   - Monitors server behavior and records information and errors

## Requirements

- C++ compiler with C++11 support or later
- Boost.Asio library
- spdlog library

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/secure-http-server.git
   ```

2. Install dependencies (Boost.Asio and spdlog)

3. Compile the project:
   ```
   g++ -std=c++11 main.cpp -o http_server -lboost_system -lpthread
   ```

## Usage

Run the compiled executable:
```
./http_server
```

The server will start and listen on port 8080 by default.

## Future Improvements

- Implement user authentication mechanisms
- Add proper authorization checks
- Enable HTTPS encryption using SSL/TLS certificates
- Conduct regular security audits and vulnerability assessments
- Perform penetration testing
- Keep the server and its dependencies up to date with security patches

## Contributing

Contributions to improve the server's security and functionality are welcome. Please feel free to submit issues or pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Contact

Praise Ojerinola - Ojerinolapraise@gmail.com

Project Link: https://github.com/praiseOjay/HTTP_Web-Server_Prototype.git
