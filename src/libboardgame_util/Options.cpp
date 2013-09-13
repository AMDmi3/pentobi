//-----------------------------------------------------------------------------
/** @file libboardgame_util/Options.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Options.h"

namespace libboardgame_util {

//----------------------------------------------------------------------------

Options::Options(int argc, const char** argv, const vector<string>& specs)
{
    init(argc, argv, specs);
}

Options::Options(int argc, char** argv, const vector<string>& specs)
{
    init(argc, const_cast<const char**>(argv), specs);
}

void Options::init(int argc, const char** argv, const vector<string>& specs)
{
    for (const auto& s : specs)
    {
        auto pos = s.find("|");
        if (pos == string::npos)
            pos = s.find(":");
        if (pos != string::npos)
            m_names.insert(s.substr(0, pos));
        else
            m_names.insert(s);
    }

    bool end_of_options = false;
    for (int n = 1; n < argc; ++n)
    {
        const string arg = argv[n];
        if (! end_of_options && arg.find("-") == 0 && arg != "-")
        {
            if (arg == "--")
            {
                end_of_options = true;
                continue;
            }
            string name;
            string value;
            bool needs_arg = false;
            if (arg.find("--") == 0)
            {
                // Long option
                name = arg.substr(2);
                auto sz = name.size();
                bool found = false;
                for (auto& spec : specs)
                    if (spec.find(name) == 0
                        && (spec.size() == sz || spec[sz] == '|'
                            || spec[sz] == ':' ))
                    {
                        found = true;
                        needs_arg = (! spec.empty() && spec.back() == ':');
                        break;
                    }
                if (! found)
                    throw OptionError("Unknown option " + arg);
            }
            else
            {
                // Short options
                for (unsigned i = 1; i < arg.size(); ++i)
                {
                    auto c = arg[i];
                    bool found = false;
                    for (auto& spec : specs)
                    {
                        auto pos = spec.find("|" + string(1, c));
                        if (pos != string::npos)
                        {
                            name = spec.substr(0, pos);
                            found = true;
                            if (! spec.empty() && spec.back() == ':')
                            {
                                // If not last option, no space was used to
                                // append the value
                                if (i != arg.size() - 1)
                                    value = arg.substr(i + 1);
                                else
                                    needs_arg = true;
                            }
                            break;
                        }
                    }
                    if (! found)
                        throw OptionError("Unknown option -" + string(1, c));
                    if (needs_arg || ! value.empty())
                        break;
                    m_map.insert(make_pair(name, ""));
                }
            }
            if (needs_arg)
            {
                bool value_found = false;
                ++n;
                if (n < argc)
                {
                    value = argv[n];
                    if (value.find("-") != 0)
                        value_found = true;
                }
                if (! value_found)
                    throw OptionError("Option --" + name + " needs value");
            }
            m_map.insert(make_pair(name, value));
        }
        else
            m_args.push_back(arg);
    }
}

void Options::check_name(const string& name) const
{
    if (m_names.find(name) == m_names.end())
        throw OptionError("Internal error: invalid option name " + name);
}

string Options::get(const string& name) const
{
    check_name(name);
    auto pos = m_map.find(name);
    if (pos == m_map.end())
        throw OptionError("Missing option --" + name);
    return pos->second;
}

string Options::get(const string& name, const string& default_value) const
{
    check_name(name);
    auto pos = m_map.find(name);
    if (pos == m_map.end())
        return default_value;
    return pos->second;
}

string Options::get(const string& name, const char* default_value) const
{
    return get(name, string(default_value));
}

//----------------------------------------------------------------------------

} // namespace libboardgame_util
