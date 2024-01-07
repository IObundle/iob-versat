{ pkgs ? import (builtins.fetchTarball {
  # Descriptive name to make the store path easier to identify
  name = "nixos-22.11";
  # Commit hash for nixos-22.11
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/22.11.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "11w3wn2yjhaa5pv20gbfbirvjq6i3m7pqrq2msf0g7cv44vijwgw";
}) {}}:
pkgs.stdenv.mkDerivation rec {
  pname = "versat";
  version = "0.1";

  src = builtins.path {
    name = pname;
    path = ./.;
  };

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

  buildPhase = ''
    #make clean
    make -j versat
  '';

  installPhase = ''
    mkdir -p $out/bin
    mv versat $out/bin
  '';

  dontStrip = true;
}

