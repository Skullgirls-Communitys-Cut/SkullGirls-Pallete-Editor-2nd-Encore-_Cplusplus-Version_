#pragma once

class DirectXWrapper {

private:
    // Singletone
    static DirectXWrapper*& instancePtr() {
        static DirectXWrapper* instance = nullptr;
        return instance;
    }
public:
    static bool Init();
};

