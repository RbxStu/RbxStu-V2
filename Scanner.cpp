//
// Created by Dottik on 10/8/2024.
//

#include "Scanner.hpp"

std::shared_ptr<Scanner> Scanner::pInstance;

std::shared_ptr<Scanner> Scanner::GetSingleton() {
    if (Scanner::pInstance == nullptr)
        Scanner::pInstance = std::make_shared<Scanner>();

    return Scanner::pInstance;
}
