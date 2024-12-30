#ifndef ILLUMINA_PARSEHELPER_H
#define ILLUMINA_PARSEHELPER_H

#include <string_view>
#include <string>

namespace illumina {

class ParseHelper {
public:
    std::string_view read_chunk();
    void rewind_all();
    bool finished() const;
    std::string_view remainder() const;

    explicit ParseHelper(std::string_view sv, size_t offset = 0);

private:
    std::string_view m_str;
    size_t m_pos = 0;
    size_t m_offset = 0;
};

inline bool ParseHelper::finished() const {
    return m_pos >= m_str.size();
}

inline std::string_view ParseHelper::read_chunk() {
    // Skip trailing whitespace and get to the next chunk.
    while (m_pos < m_str.size()) {
        if (!std::isspace(m_str[m_pos])) {
            break;
        }
        m_pos++;
    }

    // Read the string until whitespace is found (chunk ends).
    size_t start = m_pos;
    while (m_pos < m_str.size()) {
        if (std::isspace(m_str[m_pos])) {
            break;
        }
        m_pos++;
    }

    return m_str.substr(start, m_pos - start);
}

inline void ParseHelper::rewind_all() {
    m_pos = m_offset;
}

inline std::string_view ParseHelper::remainder() const {
    return m_str.substr(m_pos, m_str.size() - m_pos - m_offset);
}

inline ParseHelper::ParseHelper(std::string_view sv, size_t offset)
    : m_str(sv), m_pos(offset), m_offset(offset) {}

} // illumina

#endif // ILLUMINA_PARSEHELPER_H
