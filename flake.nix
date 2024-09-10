{
  description = "C++ development environment";

  inputs = {
    nixpkgs = { url = "github:nixos/nixpkgs/nixos-unstable"; };
  };

  outputs = { self, nixpkgs }: 
  let 
    supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
    
    # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
    forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

    # Nixpkgs instantiated for supported system types.
    pkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
  in
  {
    devShells = forAllSystems (system: {
      default =
        pkgsFor.${system}.mkShell.override {
          stdenv = pkgsFor.${system}.gcc14Stdenv;
        } {

          name = "c++-dev-shell";
          hardeningDisable = ["all"];
          packages = with pkgsFor.${system}; [
            gcc14                   # compiler
            cmake                   # build system
            valgrind                # memory debugger
            clang-tools             # code formatting
          ];
          shellHook = ''
              zsh
          '';

          LD_LIBRARY_PATH="${pkgsFor.${system}.libz}/lib";
          CXX="${pkgsFor.${system}.gcc14}/bin/g++";
        };
    });
  };
}
