#include "session_manager.hpp"
#include "csv_parser.hpp"

#include <stdexcept>

namespace Telemetry {

const std::vector<LapSession>& SessionManager::sessions() const noexcept {
    return m_sessions;
}

bool SessionManager::hasSessions() const noexcept {
    return !m_sessions.empty();
}

size_t SessionManager::sessionCount() const noexcept {
    return m_sessions.size();
}

LapSession* SessionManager::sessionAt(size_t index) noexcept {
    if (index >= m_sessions.size()) {
        return nullptr;
    }
    return &m_sessions[index];
}

const LapSession* SessionManager::sessionAt(size_t index) const noexcept {
    if (index >= m_sessions.size()) {
        return nullptr;
    }
    return &m_sessions[index];
}

LapSession* SessionManager::sessionByName(const std::string& name) noexcept {
    for (auto& session : m_sessions) {
        if (session.lapName() == name) {
            return &session;
        }
    }
    return nullptr;
}

const LapSession* SessionManager::sessionByName(const std::string& name) const noexcept {
    for (const auto& session : m_sessions) {
        if (session.lapName() == name) {
            return &session;
        }
    }
    return nullptr;
}

const LapSession& SessionManager::loadSession(const std::string& path, const std::string& lapName) {
    Telemetry::CsvParser parser;
    auto records = parser.parse(path);
    if (records.empty()) {
        throw std::runtime_error("Falha ao carregar sessão: arquivo CSV vazio ou inválido.");
    }
    m_sessions.emplace_back(lapName.empty() ? path : lapName, std::move(records));
    return m_sessions.back();
}

} // namespace Telemetry
