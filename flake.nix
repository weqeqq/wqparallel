{
  description = "wqparallel: C++23 parallel library with thread pool and parallel algorithms";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      devShells = forEachSystem (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          llvm = pkgs.llvmPackages_latest;

          clangLibcxx = pkgs.symlinkJoin {
            name = "clang-libcxx-${llvm.clang.version}";
            paths = [
              llvm.libcxxClang
              llvm.libcxx
              llvm.libcxx.dev
              llvm.clang-tools
            ];
          };
        in
        {
          default = pkgs.mkShell.override { stdenv = llvm.libcxxStdenv; } {
            hardeningDisable = [ "fortify" ];

            packages = [
              clangLibcxx
              llvm.clang-tools
              llvm.lldb
              pkgs.xmake
              pkgs.ninja
            ];

            shellHook = ''
              export CC=clang
              export CXX=clang++
            '';
          };
        }
      );
    };
}
