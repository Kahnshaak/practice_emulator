{
  description = "Devshell for building the emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };

	# Should be pinned versions from the lock file and the nixpkgs version
        gcc = pkgs.gcc11; 
        cmake = pkgs.cmake;
        python = pkgs.python310Full;
        make = pkgs.gnumake;

      in {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            gcc
            cmake
            make
            python
	    git
          ];

          shellHook = ''
            echo "Devshell for emulator"
            echo "g++: $(g++ --version | head -n1)"
            echo "python: $(python --version)"
            echo "cmake: $(cmake --version | head -n1)"
            echo "make: $(make --version | head -n1)"
          '';
        };
      });
}

