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
sourceFiles = fs.union ./Makefile (fs.union ./sharedHardware.mk (fs.union ./config.mk ./software));
in
#fs.trace {} sourceFiles
pkgs.stdenv.mkDerivation rec {
  pname = "versat";
  version = "0.1";

  src = fs.toSource{
    root = ./.;
    fileset = sourceFiles;
  };

  #src = ./.;

  #srcs = builtins.path {
  #  name = pname;
  #  path = ./.;
  #};

  #src = pkgs.fetchgit {
  #  url = "https://github.com/zettasticks/iob-versat.git";
  #  rev = "4be8fe2f1a44ac77cf9bcb4c44f0734e928fec2d";
  #  fetchSubmodules = true;
  #  deepClone = true;
  #  sha256 = "sha256-y5s5fwAQh5x2NNB2jUgN/MyoNdqtOQEuPcBd3v5AA6Q=";
  #};

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
