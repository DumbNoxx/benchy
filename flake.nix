{
  description = "Goxe";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        nativeBuildInputs = [
          pkgs.gcc
          pkgs.cmake
          pkgs.gdb
          pkgs.valgrind
          pkgs.clang-tools
          pkgs.glibc.static
          pkgs.bear
        ];
        buildInputs = with pkgs; [ ];
      in
      {
        devShells.default = pkgs.mkShell { inherit nativeBuildInputs buildInputs; };

      }
    );
}
