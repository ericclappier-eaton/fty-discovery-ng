#include "credentials.h"

#include "src/config.h"

#include <fty_security_wallet.h>
#include <fty_common_socket_sync_client.h>

namespace fty::disco::impl {

    static secw::DocumentPtr getCredentialsWithSecw(const std::string& id) {
        fty::SocketSyncClient secwSyncClient(Config::instance().secwSocket.value());
        auto         client = secw::ConsumerAccessor(secwSyncClient);

        return client.getDocumentWithPrivateData("default", id);
    }

    static std::function<secw::DocumentPtr(const std::string&)> g_FctGetCredential = getCredentialsWithSecw;

    void setCredentialsService(std::function<secw::DocumentPtr(const std::string&)> fct){

        if(fct == nullptr) {
            g_FctGetCredential = getCredentialsWithSecw;
        } else {
            g_FctGetCredential = fct;
        }
        
    }

    secw::DocumentPtr getCredential(const std::string& id) {
        return g_FctGetCredential(id);
    }
}