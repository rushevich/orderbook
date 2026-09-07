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
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            clang-tools
            gdb
            linuxPackages_latest.perf
            hyperfine
          ];

          buildInputs = with pkgs; [
            gtest
            gbenchmark
          ];

          shellHook = ''
            echo "orderbook devshell: $(c++ --version | head -n1)"
          '';
        };
      }
    );
}
