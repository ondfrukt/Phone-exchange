

// Om jag vill i denna klass använda använda funktuoner ur klassen LineManager
// så skapar jag en referens till den som skapats i App.h

#pragma once
class LineManager; // Forward declaration of LineManager class

class WebServer {
  public:
    WebServer(LineManager& lineManager); // Constructor that takes a reference to LineManager

    void start(); // Method to start the web server
    void handleClient(); // Method to handle incoming client requests
private:
    LineManager& lineMgr_;  // referarar till LineManager-objektet i App.h
};