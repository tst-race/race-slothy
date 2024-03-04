#ifndef __SLOTHY_HTTP_H__
#define __SLOTHY_HTTP_H__

#include "slothy.hpp"

class SlothyHTTP : public Slothy {
  public:
    SlothyHTTP(std::string registry_address);
    // virtual ~SlothyHTTP(){ }

  protected:

  private:
      nlohmann::json get_artifact_registry_info(std::string artifact_name) override;

    //
};

#endif