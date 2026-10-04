// Minh Thu
#include <filesystem>
#include <iostream>

#include "DangKhamManager.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 1 && argc != 3)
    {
        cerr
            << "Cach dung: DangKham.exe "
            << "[truyXuat.db dangKham.db]\n";

        return 1;
    }

    const filesystem::path source =
        argc == 3
            ? argv[1]
            : "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const filesystem::path destination =
        argc == 3
            ? argv[2]
            : "DANG_KHAM/db/dangKham.db";

    try
    {
        if (!filesystem::is_regular_file(source))
        {
            cerr
                << "Khong tim thay database nguon: "
                << source
                << '\n';

            return 1;
        }

        if (!destination.parent_path().empty())
        {
            filesystem::create_directories(
                destination.parent_path()
            );
        }
    }
    catch (const filesystem::filesystem_error& error)
    {
        cerr << error.what() << '\n';
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

    cout << "Da thoat DANG_KHAM.\n";
    return 0;
}
