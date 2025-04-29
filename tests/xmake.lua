target("echo_server", function()
    set_kind("binary")
    add_files("echo_server.cpp")
    add_deps("cutemuduo")
end)

target("echo_client", function()
    set_kind("binary")
    add_files("echo_client.cpp")
end)

-- add_deps("cutemuduo")

-- for _, sourcefile in ipairs(os.files("*.cpp")) do
--     target(path.basename(sourcefile))
--         set_kind("binary")
--         add_files(sourcefile)
-- end

