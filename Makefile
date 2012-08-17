PROG=	httee

LIBXDG_CFLAGS!=	pkg-config --cflags libxdg-basedir
LIBXDG_LDFLAGS!=pkg-config --libs libxdg-basedir

CFLAGS+=	${LIBXDG_CFLAGS}
LDFLAGS+=	${LIBXDG_LDFLAGS}

.include <bsd.prog.mk>
