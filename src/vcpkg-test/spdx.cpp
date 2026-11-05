#include <vcpkg-test/util.h>

#include <vcpkg/dependencies.h>
#include <vcpkg/spdx.h>

using namespace vcpkg;

static Optional<std::string> port_external_ref(StringView sbom, StringView reference_type)
{
    auto parsed = Json::parse(sbom, "test").value(VCPKG_LINE_INFO);
    const auto external_refs = parsed.value.object(VCPKG_LINE_INFO)["packages"]
                                   .array(VCPKG_LINE_INFO)[0]
                                   .object(VCPKG_LINE_INFO)["externalRefs"]
                                   .array(VCPKG_LINE_INFO);
    for (const auto& external_ref_value : external_refs)
    {
        const auto external_ref = external_ref_value.object(VCPKG_LINE_INFO);
        if (external_ref["referenceType"].string(VCPKG_LINE_INFO) == reference_type)
        {
            return external_ref["referenceLocator"].string(VCPKG_LINE_INFO).to_string();
        }
    }

    return nullopt;
}

static std::string port_purl(StringView sbom) { return port_external_ref(sbom, "purl").value_or_exit(VCPKG_LINE_INFO); }

static std::string port_download_location(StringView sbom)
{
    auto parsed = Json::parse(sbom, "test").value(VCPKG_LINE_INFO);
    return parsed.value.object(VCPKG_LINE_INFO)["packages"]
        .array(VCPKG_LINE_INFO)[0]
        .object(VCPKG_LINE_INFO)["downloadLocation"]
        .string(VCPKG_LINE_INFO)
        .to_string();
}

TEST_CASE ("spdx purl omits empty version", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"", 1};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib?triplet=arm-uwp");
}

TEST_CASE ("spdx purl percent-encodes the version", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"1.0+r2", 0};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib@1.0%2Br2?triplet=arm-uwp");
}

TEST_CASE ("spdx purl includes vcs_url for a builtin git-tree", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    scfl.kind = PortSourceKind::Builtin;
    scfl.spdx_location = "git+https://github.com/Microsoft/vcpkg@84a143e4caf6b70db57f28d04c41df4a85c480fa";
    scfl.git_tree = "84a143e4caf6b70db57f28d04c41df4a85c480fa";
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"1.0", 0};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    // The default microsoft/vcpkg registry is Builtin, so the locator has no repository_url.
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib@1.0?triplet=arm-uwp"
                             "&vcs_url=git%2Bhttps%3A%2F%2Fgithub.com%2FMicrosoft%2Fvcpkg%40"
                             "84a143e4caf6b70db57f28d04c41df4a85c480fa");
    CHECK(port_download_location(sbom) ==
          "git+https://github.com/Microsoft/vcpkg@84a143e4caf6b70db57f28d04c41df4a85c480fa");
    CHECK(port_external_ref(sbom, "gitoid").value_or_exit(VCPKG_LINE_INFO) ==
          "gitoid:tree:sha1:84a143e4caf6b70db57f28d04c41df4a85c480fa");
}

TEST_CASE ("spdx purl includes repository_url and vcs_url for a non-default git registry", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    scfl.kind = PortSourceKind::Git;
    scfl.spdx_location = "git+https://github.com/azure-sdk/vcpkg@84a143e4caf6b70db57f28d04c41df4a85c480fa";
    scfl.spdx_repository_url = "https://github.com/azure-sdk/vcpkg";
    scfl.git_tree = "84a143e4caf6b70db57f28d04c41df4a85c480fa";
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"1.0", 0};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    // repository_url is the registry URL with the git+ prefix and @git-tree stripped; vcs_url keeps both.
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib@1.0?repository_url=https%3A%2F%2Fgithub.com%2Fazure-sdk%2Fvcpkg"
                             "&triplet=arm-uwp"
                             "&vcs_url=git%2Bhttps%3A%2F%2Fgithub.com%2Fazure-sdk%2Fvcpkg%40"
                             "84a143e4caf6b70db57f28d04c41df4a85c480fa");
    CHECK(port_download_location(sbom) ==
          "git+https://github.com/azure-sdk/vcpkg@84a143e4caf6b70db57f28d04c41df4a85c480fa");
    CHECK(port_external_ref(sbom, "gitoid").value_or_exit(VCPKG_LINE_INFO) ==
          "gitoid:tree:sha1:84a143e4caf6b70db57f28d04c41df4a85c480fa");
}

TEST_CASE ("spdx purl uses the dedicated repository_url field for git registries", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    scfl.kind = PortSourceKind::Git;
    scfl.spdx_location = "git+https://github.com/azure-sdk/vcpkg";
    scfl.spdx_repository_url = "https://example.com/registry";
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"1.0", 0};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib@1.0?repository_url=https%3A%2F%2Fexample.com%2Fregistry"
                             "&triplet=arm-uwp"
                             "&vcs_url=git%2Bhttps%3A%2F%2Fgithub.com%2Fazure-sdk%2Fvcpkg");
}

TEST_CASE ("spdx purl omits vcs_url and repository_url for overlay ports", "[spdx]")
{
    PackagesDirAssigner packages_dir_assigner{"test_packages_root"};
    PackageSpec spec{"zlib", Test::ARM_UWP};
    SourceControlFileAndLocation scfl;
    scfl.kind = PortSourceKind::Overlay;
    auto& scf = *(scfl.source_control_file = std::make_unique<SourceControlFile>());
    auto& cpgh = *(scf.core_paragraph = std::make_unique<SourceParagraph>());
    cpgh.name = "zlib";
    cpgh.version = Version{"1.0", 0};

    InstallPlanAction ipa(
        spec, scfl, packages_dir_assigner, RequestType::USER_REQUESTED, UseHeadVersion::No, Editable::No, {}, {}, {});
    auto& abi = *(ipa.abi_info = AbiInfo{}).get();
    abi.package_abi = "ABIHASH";

    const auto sbom = create_spdx_sbom(ipa, {}, {}, {}, {}, "now", "https://test-document-namespace", {});
    CHECK(port_purl(sbom) == "pkg:vcpkg/zlib@1.0?triplet=arm-uwp");
    CHECK(!port_external_ref(sbom, "gitoid").has_value());
}

TEST_CASE ("spdx license parse edge cases", "[spdx]")
{
    CHECK(!read_spdx_license_text("this is not json", "test").has_value());

    static constexpr StringLiteral missing_packages = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(missing_packages, "test").has_value());

    static constexpr StringLiteral empty_packages = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": [],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(empty_packages, "test").has_value());

    static constexpr StringLiteral wrong_packages_type = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": {},
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(wrong_packages_type, "test").has_value());

    static constexpr StringLiteral wrong_packages_zero_type = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": [42],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(wrong_packages_zero_type, "test").has_value());

    static constexpr StringLiteral missing_license_block = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": [
    {
      "name": "zlib",
      "SPDXID": "SPDXRef-port",
      "versionInfo": "1.0",
      "downloadLocation": "NOASSERTION",
      "copyrightText": "NOASSERTION",
      "comment": "This is the port (recipe) consumed by vcpkg."
    }
  ],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(missing_license_block, "test").has_value());

    static constexpr StringLiteral wrong_license_type = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": [
    {
      "name": "zlib",
      "SPDXID": "SPDXRef-port",
      "versionInfo": "1.0",
      "downloadLocation": "NOASSERTION",
      "licenseConcluded": 42,
      "licenseDeclared": 42,
      "copyrightText": "NOASSERTION",
      "comment": "This is the port (recipe) consumed by vcpkg."
    }
  ],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(wrong_license_type, "test").has_value());

    static constexpr StringLiteral empty_license = R"json(
{
  "$schema": "https://raw.githubusercontent.com/spdx/spdx-spec/v2.2.1/schemas/spdx-schema.json",
  "spdxVersion": "SPDX-2.2",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "documentNamespace": "https://test-document-namespace-2",
  "name": "zlib:arm-uwp@1.0 deadbeef",
  "creationInfo": {
    "creators": [
      "Tool: vcpkg-2999-12-31-unknownhash"
    ],
    "created": "now+1"
  },
  "relationships": [
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "GENERATES",
      "relatedSpdxElement": "SPDXRef-binary"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-0"
    },
    {
      "spdxElementId": "SPDXRef-port",
      "relationshipType": "CONTAINS",
      "relatedSpdxElement": "SPDXRef-file-1"
    },
    {
      "spdxElementId": "SPDXRef-binary",
      "relationshipType": "GENERATED_FROM",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-0",
      "relationshipType": "DEPENDENCY_MANIFEST_OF",
      "relatedSpdxElement": "SPDXRef-port"
    },
    {
      "spdxElementId": "SPDXRef-file-1",
      "relationshipType": "CONTAINED_BY",
      "relatedSpdxElement": "SPDXRef-port"
    }
  ],
  "packages": [
    {
      "name": "zlib",
      "SPDXID": "SPDXRef-port",
      "versionInfo": "1.0",
      "downloadLocation": "NOASSERTION",
      "licenseConcluded": "",
      "licenseDeclared": "",
      "copyrightText": "NOASSERTION",
      "comment": "This is the port (recipe) consumed by vcpkg."
    },
    {
      "name": "zlib:arm-uwp",
      "SPDXID": "SPDXRef-binary",
      "versionInfo": "deadbeef",
      "downloadLocation": "NONE",
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION",
      "comment": "This is a binary package built by vcpkg."
    }
  ],
  "files": [
    {
      "fileName": "./vcpkg.json",
      "SPDXID": "SPDXRef-port-file-0",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-vcpkg.json"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    },
    {
      "fileName": "./portfile.cmake",
      "SPDXID": "SPDXRef-port-file-1",
      "checksums": [
        {
          "algorithm": "SHA256",
          "checksumValue": "hash-portfile.cmake"
        }
      ],
      "licenseConcluded": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ]
})json";

    CHECK(!read_spdx_license_text(empty_license, "test").has_value());
}
