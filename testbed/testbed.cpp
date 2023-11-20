
#include <gfs/gfs.hpp>

#include <cassert>
#include <iostream>
#include <filesystem>

int main()
{
    std::cout << "GFS Testbed\n";

    std::filesystem::create_directories("mount_a/aa/ab");
    std::filesystem::create_directories("mount_b_locked");

    gfs::Filesystem fs;

    auto mountA = fs.MountDir("mount_a", true);
    assert(mountA != gfs::InvalidMountId);
    auto mountB = fs.MountDir("mount_b_locked", false);
    assert(mountB != gfs::InvalidMountId);

    assert(fs.IsPathInAnyMount("aa/ab"));
    assert(!fs.IsPathInAnyMount("some_file.txt"));
    assert(!fs.IsPathInAnyMount(".././some_other_file.txt"));

    return 0;
}