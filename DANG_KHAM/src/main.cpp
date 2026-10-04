// Minh Thu
#include <filesystem>
#include <iostream>

#include "DangKhamManager.h"

int main(int argc, char* argv[])
{
    if (argc != 1 && argc != 3)
    {
        std::cerr
            << "Cach dung: DangKham.exe "
            << "[truyXuat.db dangKham.db]\n";

        return 1;
    }

    const std::filesystem::path source =
        argc == 3
            ? argv[1]
            : "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const std::filesystem::path destination =
        argc == 3
            ? argv[2]
            : "DANG_KHAM/db/dangKham.db";

    try
    {
        if (!std::filesystem::is_regular_file(source))
        {
            std::cerr
                << "Khong tim thay database nguon: "
                << source
                << '\n';

            return 1;
        }

        if (!destination.parent_path().empty())
        {
            std::filesystem::create_directories(
                destination.parent_path()
            );
        }
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    DangKhamManager manager;

    if (!manager.khoiDong(
            source.string(),
            destination.string()
        ))
    {
        return 1;
    }

    manager.chayMenu();

    std::cout << "Da thoat DANG_KHAM.\n";
    return 0;
}
