#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace AliasManager {

// Initializes aliases from workspace .portpeek or user global config
void Initialize();

// Reloads aliases from disk
void Reload();

// Gets custom alias for a port (e.g. 5432 -> "Main Postgres DB"), returns empty if none
std::wstring GetAliasForPort(uint16_t port);

// Sets an alias and saves to user config
void SetAlias(uint16_t port, const std::wstring& name);

// Returns all registered aliases
std::unordered_map<uint16_t, std::wstring> GetAllAliases();

} // namespace AliasManager
