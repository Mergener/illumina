#ifndef ILLUMINA_UCIOPTION_H
#define ILLUMINA_UCIOPTION_H

#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <functional>

#include "illumina.h"

namespace illumina {

class UCIOption;

using UCIOptionUpdateHandler = std::function<void(UCIOption&)>;

class UCIOption {
public:
    void parse_and_set(std::string_view str);
    void add_update_handler(const UCIOptionUpdateHandler& handler);

    virtual std::string type_name() const = 0;
    virtual std::string default_value_str() const = 0;
    virtual std::string current_value_str() const = 0;
    virtual bool        has_value() const;     // Defaults to returning true.
    virtual bool        has_min_value() const; // Defaults to returning false.
    virtual bool        has_max_value() const; // Defaults to returning false.
    virtual std::string min_value_str() const;
    virtual std::string max_value_str() const;

    const std::string& name() const;

    explicit UCIOption(std::string name);
    virtual ~UCIOption() = default;

protected:
    virtual void parse_and_set_handler(std::string_view str) = 0;

private:
    std::string m_name;
    std::vector<UCIOptionUpdateHandler> m_update_handlers;
};

class UCIOptionString : public UCIOption {
public:
    const std::string& value() const;
    const std::string& default_value() const;

    void        parse_and_set_handler(std::string_view str) override;
    std::string default_value_str() const override;
    std::string current_value_str() const override;
    std::string type_name() const override;

    UCIOptionString(std::string name, std::string default_val);

private:
    std::string m_val;
    std::string m_default_val;
};

class UCIOptionSpin : public UCIOption {
public:
    i64 value() const;
    i64 default_value() const;
    i64 min_value() const;
    i64 max_value() const;

    void        parse_and_set_handler(std::string_view str) override;
    std::string default_value_str() const override;
    std::string current_value_str() const override;
    bool        has_min_value() const override;
    bool        has_max_value() const override;
    std::string min_value_str() const override;
    std::string max_value_str() const override;
    std::string type_name() const override;

    UCIOptionSpin(std::string name,
                  i64 default_val,
                  i64 min_val,
                  i64 max_val);

private:
    i64 m_val;
    i64 m_default_val;
    i64 m_min_val;
    i64 m_max_val;
};

class UCIOptionCheck : public UCIOption {
public:
    bool value() const;
    bool default_value() const;

    void        parse_and_set_handler(std::string_view str) override;
    std::string default_value_str() const override;
    std::string current_value_str() const override;
    std::string type_name() const override;

    UCIOptionCheck(std::string name, bool default_val);

private:
    bool m_val;
    bool m_default_val;
};

class UCIOptionCombo : public UCIOptionString {
public:
    void        parse_and_set_handler(std::string_view str) override;
    std::string type_name() const override;

    UCIOptionCombo(std::string name,
                   std::string default_val,
                   std::vector<std::string> options);

    template <typename... TArgs>
    UCIOptionCombo(std::string name,
                   std::string default_val,
                   TArgs&&... args);

private:
    std::vector<std::string> m_opts;
};

class UCIOptionButton : public UCIOption {
public:
    void         parse_and_set_handler(std::string_view str) override;
    std::string  default_value_str() const override;
    std::string  current_value_str() const override;
    bool has_value() const override;
    std::string  type_name() const override;

    explicit UCIOptionButton(std::string name);
};

class UCIOptionManager {
public:
    template <typename TOption, typename... TArgs>
    TOption& register_option(const std::string& name, TArgs&&... args);

    UCIOption&       option(std::string_view name);
    const UCIOption& option(std::string_view name) const;

    using OptionList = std::vector<std::reference_wrapper<UCIOption>>;
    OptionList list_options() const;

private:
    std::map<std::string, std::unique_ptr<UCIOption>>  m_options;
};

template <typename... TArgs>
inline UCIOptionCombo::UCIOptionCombo(std::string name,
                                      std::string default_val,
                                      TArgs&&... args)
   : UCIOptionString(name, default_val), m_opts({args...}) { }

template <typename TOption, typename... TArgs>
inline TOption& UCIOptionManager::register_option(const std::string& name, TArgs&&... args) {
    static_assert(std::is_base_of_v<UCIOption, TOption>, "TOption must be derived from UCIOption.");

    std::unique_ptr<TOption> option_ptr = std::make_unique<TOption>(name, args...);
    TOption& option = *option_ptr;
    m_options[name] = std::move(option_ptr);

    return option;
}

} // illumina

#endif // ILLUMINA_UCIOPTION_H
