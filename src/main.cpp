#include <oak/oak.hpp>

int main()
{
    oak::init_writer();

    auto r = oak::settings_file("settings.oak");
    if (!r.has_value())
    {
        oak::error("Error opening setting file: {}", r.error());
        return 1;
    }

    std::string name = "Mario";
    oak::out("Hello {}", name);

    oak::stop_writer();
    return 0;
}
