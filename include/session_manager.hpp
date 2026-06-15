#pragma once

#include "lap_session.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Telemetry {

class SessionManager {
public:
    SessionManager() = default;

    const std::vector<LapSession>& sessions() const noexcept;
    bool hasSessions() const noexcept;
    size_t sessionCount() const noexcept;

    LapSession* sessionAt(size_t index) noexcept;
    const LapSession* sessionAt(size_t index) const noexcept;
    LapSession* sessionByName(const std::string& name) noexcept;
    const LapSession* sessionByName(const std::string& name) const noexcept;

    const LapSession& loadSession(const std::string& path, const std::string& lapName);

private:
    std::vector<LapSession> m_sessions;
};

} // namespace Telemetry
