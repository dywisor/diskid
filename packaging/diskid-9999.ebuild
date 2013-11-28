EAPI=5

EGIT_REPO_URI="git://github.com/dywisor/${PN}.git"

inherit base git-r3

DESCRIPTION="read disk identifiers from devices"
HOMEPAGE="https://github.com/dywisor/diskid"
SRC_URI=""

LICENSE="GPL-2+ LGPL-2.1+"
SLOT="0"
IUSE="minimal static"

KEYWORDS=""

DEPEND=""
RDEPEND=""

src_compile() {
	emake MINIMAL=$(usex minimal 1 0) STATIC=$(usex static 1 0) "${PN}"
}
