{ pkgs ? import <nixpkgs> {
  overlays = [ 
    (self: super: {
      xfstests = super.xfstests.overrideAttrs (super: {
        version = "git";
        src = /home/alberand/Projects/xfstests-dev;
        postInstall = super.postInstall + ''
                cp ${./xfstests-config} $out/xfstests-config
        '';
      });
    })
  ];
} }:

pkgs.stdenv.mkDerivation {
  name = "xfstests";
  nativeBuildInputs = with pkgs; [
    gnumake
    clang
    clang-tools
    file
    automake
    autoconf

    # xfstests
    e2fsprogs
    attr
    acl
    libaio
    keyutils
    fsverity-utils
    ima-evm-utils
    util-linux
    stress-ng
    dbench
    xfsprogs
    fio
    linuxquota
    nvme-cli
  ];
}
