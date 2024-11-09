{ pkgs ? import (builtins.fetchTarball {
  name = "nixos-22.11";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/22.11.tar.gz";
  sha256 = "11w3wn2yjhaa5pv20gbfbirvjq6i3m7pqrq2msf0g7cv44vijwgw";
}){} , 
  tweag ? import (builtins.fetchTarball {
  name = "lib-filesets";
  url = "https://github.com/tweag/nixpkgs/tarball/file-sets";
}){} 
}:
let pkgs2 = import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/cf8cc1201be8bc71b7cbbbdaf349b22f4f99c7ae.tar.gz") {};
in
let
fs = tweag.lib.fileset;
sourceFiles = (fs.union ./Makefile (fs.union ./config.mk ./software));
#myPython = pkgs2.mkShell {   
#  packages = [
#    (pkgs2.python3.withPackages (python-pkgs: with python-pkgs; [
#      # select Python packages here
#      libclang
#    ]))
#  ];
#};
in
pkgs.stdenv.mkDerivation rec {
  pname = "versat";
  version = "0.1";

  src = fs.toSource{
    root = ./.;
    fileset = sourceFiles;
  };

  buildInputs = [
    pkgs.gnumake
    pkgs.verilator
    (pkgs2.python3.withPackages (python-pkgs: with python-pkgs; [
      # select Python packages here
      libclang
    ]))  
  ];

  enableParallelBuilding = true;

  buildPhase = ''
    make -j versat
  '';

  installPhase = ''
    mkdir -p $out/bin
    mv versat $out/bin
  '';

  dontStrip = true;
}
