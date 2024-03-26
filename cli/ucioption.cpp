#include <string_view>
#include "ucioption.h"

namespace illumina {

//
// UCIOption
//

void UCIOption::parse_and_set(std::string_view str) {
    try {
        parse_and_set_handler(str);
    }
    catch (const std::exception& e) {
        std::cerr << "Error when trying to parse value for option " << name() << ": " << e.what() << std::endl;
        return;
    }

    // Notify update handlers.
    for (const UCIOptionUpdateHandler& handler: m_update_handlers) {
        handler(*this);
    }
}

void UCIOption::add_update_handler(const UCIOptionUpdateHandler& handler) {
    m_update_handlers.push_back(handler);
    handler(*this);
}

bool UCIOption::has_value() const {
    return true;
}

bool UCIOption::has_min_value() const {
    return false;
}

bool UCIOption::has_max_value() const {
    return false;
}

std::string UCIOption::min_value_str() const {
    return "";
}

std::string UCIOption::max_value_str() const {
    return "";
}

const std::string& UCIOption::name() const {
    return m_name;
}

UCIOption::UCIOption(std::string name) : m_name(std::move(name)) {}

//
// UCIOptionString
//

const std::string& UCIOptionString::value() const {
    return m_val;
}

const std::string& UCIOptionString::default_value() const {
    return m_default_val;
}

void UCIOptionString::parse_and_set_handler(std::string_view str) {
    m_val = str;
}

std::string UCIOptionString::default_value_str() const {
    return default_value();
}

std::string UCIOptionString::current_value_str() const {
    return value();
}

std::string UCIOptionString::type_name() const {
    return "string";
}

UCIOptionString::UCIOptionString(std::string name, std::string default_val)
    : UCIOption(std::move(name)),
      m_val(std::move(default_val)),
      m_default_val(std::move(default_val)) {}

//
// UCIOptionSpin
//

i64 UCIOptionSpin::value() const {
    return m_val;
}

i64 UCIOptionSpin::default_value() const {
    return m_default_val;
}

i64 UCIOptionSpin::min_value() const {
    return m_min_val;
}

i64 UCIOptionSpin::max_value() const {
    return m_max_val;
}

void UCIOptionSpin::parse_and_set_handler(std::string_view str) {
    i64 parsed_val = parse_int(str);
    if (parsed_val < min_value() || parsed_val > max_value()) {
        throw std::invalid_argument(std::to_string(parsed_val) + " is out of expected bounds ("
                                    + std::to_string(min_value()) + ", "
                                    + std::to_string(max_value()) + ")");
    }
    m_val = parsed_val;
}

std::string UCIOptionSpin::default_value_str() const {
    return std::to_string(m_default_val);
}

std::string UCIOptionSpin::current_value_str() const {
    return std::to_string(m_val);
}

std::string UCIOptionSpin::type_name() const {
    return "spin";
}

bool UCIOptionSpin::has_min_value() const {
    return true;
}

bool UCIOptionSpin::has_max_value() const {
    return true;
}

std::string UCIOptionSpin::min_value_str() const {
    return std::to_string(min_value());
}

std::string UCIOptionSpin::max_value_str() const {
    return std::to_string(max_value());
}


UCIOptionSpin::UCIOptionSpin(std::string name,
                             i64 default_val,
                             i64 min_val,
                             i64 max_val)
    : UCIOption(std::move(name)),
      m_val(default_val),
      m_default_val(default_val),
      m_min_val(min_val),
      m_max_val(max_val) {}

//
// UCIOptionCheck
//

bool UCIOptionCheck::value() const {
    return m_val;
}

bool UCIOptionCheck::default_value() const {
    return m_default_val;
}

void UCIOptionCheck::parse_and_set_handler(std::string_view str) {
    if (str == "true") {
        m_val = true;
    }
    else if (str == "false") {
        m_val = false;
    }
    else {
        throw std::invalid_argument("Invalid boolean value '" + std::string(str) + "'. (expected 'true' or 'false')");
    }
}

std::string UCIOptionCheck::default_value_str() const {
    return std::to_string(m_default_val);
}

std::string UCIOptionCheck::current_value_str() const {
    return std::to_string(m_val);
}

std::string UCIOptionCheck::type_name() const {
    return "check";
}

UCIOptionCheck::UCIOptionCheck(std::string name, bool default_val)
    : UCIOption(std::move(name)),
      m_val(default_val),
      m_default_val(default_val) {}

//
// UCIOptionCombo
//

void UCIOptionCombo::parse_and_set_handler(std::string_view str) {
    bool ok = false;
    for (const std::string& opt: m_opts) {
        if (opt == str) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        throw std::invalid_argument("Unexpected option '" + std::string(str) + "' for " + name() + ".");
    }

    UCIOptionString::parse_and_set_handler(str);
}

std::string UCIOptionCombo::type_name() const {
    return "combo";
}

UCIOptionCombo::UCIOptionCombo(std::string name,
                               std::string default_val,
                               std::vector<std::string> options)
    : UCIOptionString(std::move(name), std::move(default_val)),
      m_opts(std::move(options)) {}

//
// UCIOptionButton
//

void UCIOptionButton::parse_and_set_handler(std::string_view str) {
}

std::string UCIOptionButton::default_value_str() const {
    return "";
}

std::string UCIOptionButton::current_value_str() const {
    return "";
}

std::string UCIOptionButton::type_name() const {
    return "button";
}

bool UCIOptionButton::has_value() const {
    return false;
}

UCIOptionButton::UCIOptionButton(std::string name)
    : UCIOption(std::move(name)) {}

//
// UCIOptionManager
//

UCIOption& UCIOptionManager::option(std::string_view name) {
    std::string name_str = std::string(name);
    auto it = m_options.find(name_str);
    if (it == m_options.end()) {
        throw std::out_of_range("Option not found: " + name_str);
    }
    return *it->second;
}

const UCIOption& UCIOptionManager::option(std::string_view name) const {
    std::string name_str = std::string(name);
    auto it = m_options.find(name_str);
    if (it == m_options.end()) {
        throw std::out_of_range("Option not found: " + name_str);
    }
    return *it->second;
}

UCIOptionManager::OptionList UCIOptionManager::list_options() const {
    OptionList list;
    for (const auto& pair: m_options) {
        list.emplace_back(*pair.second);
    }
    return list;
}

} // illumina