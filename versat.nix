{ pkgs ? import (builtins.fetchTarball {
  name = "nixos-22.11";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/22.11.tar.gz";
  sha256 = "11w3wn2yjhaa5pv20gbfbirvjq6i3m7pqrq2msf0g7cv44vijwgw";
}){} , 
  tweag ? import (builtins.fetchTarball {
  name = "lib-filesets";
  url = "https://github.com/tweag/nixpkgs/tarball/file-sets";
#  sha256 = "";
}){} }:
let
fs = tweag.lib.fileset;
sourceFiles = (fs.union ./typeInfo.cpp (fs.union ./Makefile (fs.union ./sharedHardware.mk (fs.union ./config.mk ./software))));
in
#fs.trace {} sourceFiles
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
