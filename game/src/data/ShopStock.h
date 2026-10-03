#pragma once

#include <string>
#include <vector>

// ShopStock is the data table of inventoriable gear keys a town shop sells.
// Loaded from assets/shop/stock.json.
//
// Expected JSON schema:
// {
//   "stock": ["starter_sword", "starter_shield"]
// }
class ShopStock
{
public:
    static std::vector<std::string> load(const std::string &filePath);
};