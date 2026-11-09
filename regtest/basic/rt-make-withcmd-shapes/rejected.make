check-rejections:
	@for rejection_case in NESTED_DYNAMIC NESTED_BOOL VOID_DATA BOOL_CONTAINER; do \
	  if $(CXX) $(CPPFLAGS) $(CXXFLAGS) -D$$rejection_case -x c++ -fsyntax-only rejected.inc > rejection.log 2>&1; then \
	    cat rejection.log; echo "Unexpected compilation success: $$rejection_case"; exit 1; \
	  fi; \
	  grep -Eq 'cmd container data|cmd cannot flatten nested dynamic containers|cmd expects a scalar, pointer, fixed array' rejection.log || { cat rejection.log; exit 1; }; \
	done
	@echo 'Unsupported containers rejected: OK'
