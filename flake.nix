{
  description = "A very basic flake";

  outputs = { self, nixpkgs }: {

    packages.x86_64-linux.xfstests = with import nixpkgs { system = "x86_64-linux"; }; callPackage ./xfstests.nix { };

    packages.x86_64-linux.default = self.packages.x86_64-linux.xfstests;

  };
}
