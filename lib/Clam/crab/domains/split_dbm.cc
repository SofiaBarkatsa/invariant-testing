#include <clam/config.h>
#include <clam/CrabDomain.hh>
#include <clam/RegisterAnalysis.hh>
#include "split_dbm.hh"

namespace clam {
#ifdef INCLUDE_ALL_DOMAINS
REGISTER_DOMAIN(clam::CrabDomain::ZONES_SPLIT_DBM, split_dbm_domain)
#else
UNREGISTER_DOMAIN(split_dbm_domain)
#endif
} // end namespace clam

