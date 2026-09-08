{
  description = "devshell for LOB project";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        buildTools = with pkgs; [
          cmake
          ninja
          pkg-config
        ];

        libs = with pkgs; [
          gtest
          gbenchmark
        ];
      in
      {
        devShells = {
          ci = pkgs.mkShell {
            nativeBuildInputs = buildTools;
            buildInputs = libs;
          };

          default = pkgs.mkShell {
            nativeBuildInputs =
              buildTools
              ++ (with pkgs; [
                clang-tools
                gdb
              ])
              ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
                pkgs.linuxPackages_latest.perf
              ];
            buildInputs = libs;
            shellHook = ''
              echo "orderbook devshell: $(c++ --version | head -n1)"
            '';
          };
        };
      }
    );
}
