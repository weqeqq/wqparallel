{
  description = "wqparallel: header-only C++23 parallel helpers";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forAllSystems = f: nixpkgs.lib.genAttrs systems f;

      perSystem = system:
        let
          pkgs = import nixpkgs { inherit system; };
          lib = pkgs.lib;
          gtestPkg = if pkgs ? gtest then pkgs.gtest else pkgs.googletest;

          mkLibrary = { tests ? false }:
            pkgs.stdenv.mkDerivation {
              pname = "wqparallel";
              version = "0.2.2";
              src = ./.;

              strictDeps = true;

              nativeBuildInputs = with pkgs; [
                meson
                ninja
                pkg-config
              ];

              buildInputs = lib.optionals tests [ gtestPkg ];

              mesonFlags = [
                "-Dtests=${if tests then "enabled" else "disabled"}"
              ];

              doCheck = tests;

              meta = with lib; {
                description = "Header-only parallel helpers for C++23";
                license = licenses.mit;
                platforms = platforms.unix;
              };
            };

          package = mkLibrary { };
          checkPackage = mkLibrary { tests = true; };
        in
        {
          inherit pkgs package checkPackage;
        };
    in
    {
      packages = forAllSystems (
        system:
        let
          inherit (perSystem system) package;
        in
        {
          default = package;
          wqparallel = package;
        }
      );

      checks = forAllSystems (
        system:
        let
          inherit (perSystem system) checkPackage;
        in
        {
          default = checkPackage;
          wqparallel-tests = checkPackage;
        }
      );

      devShells = forAllSystems (
        system:
        let
          inherit (perSystem system) pkgs checkPackage;
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ checkPackage ];

            packages = with pkgs; [
              clang-tools
              doxygen
            ];
          };
        }
      );

      formatter = forAllSystems (system: (perSystem system).pkgs.nixpkgs-fmt);
    };
}
