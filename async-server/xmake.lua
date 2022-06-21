add_rules("mode.debug", "mode.release")

add_includedirs("include")

target("asyncserver")
    set_kind("static")
    add_files("server/*.cpp")
    add_linkdirs("lib")
    add_links("uring")

target("test")
    set_kind("binary")
    add_files("main.cpp")
    add_deps("asyncserver")