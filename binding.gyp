{
  "targets": [
    {
      "target_name": "beagle_carrier",
      "sources": [
        "src/addon/beagle_carrier.cc",
        "src/addon/carrier_wrapper.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "deps/elastos-carrier/include"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            "-L<(module_root_dir)/deps/elastos-carrier/lib",
            "-lelacarrier",
            "-lelasession"
          ],
          "ldflags": [
            "-Wl,-rpath,'$$ORIGIN/../deps/elastos-carrier/lib'"
          ]
        }],
        ["OS=='mac'", {
          "libraries": [
            "-L<(module_root_dir)/deps/elastos-carrier/lib",
            "-lelacarrier",
            "-lelasession"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }]
      ]
    }
  ]
}
