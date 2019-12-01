all:

readme: readme.mdoc
	mandoc -mdoc readme.mdoc | perl -e 'foreach (<STDIN>) { \
		$$_ =~ s/(.)\x08\1/$$1/g; $$_ =~ s/_\x08(.)/$$1/g; print $$_ \
	}' | sed '1,1d' > README

readmemd: readme.mdoc
	mandoc -mdoc -Tmarkdown readme.mdoc | sed '1,1d; $$,$$d' > README.md


