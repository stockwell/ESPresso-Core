#include "SettingsManager.hpp"

void SettingsManager::save()
{
    printf("Saving %zu keys..\n", m_settings.size());

    for (const auto [key, value] : m_settings)
    {
        switch (value.get().index())
        {
            case 0:
                printf("--> %s: %d\n", key.c_str(), value.getAs<int>());
                break;

            case 1:
                printf("--> %s: %f\n", key.c_str(), value.getAs<float>());
                break;

            case 2:
                printf("--> %s: %s\n", key.c_str(), value.getAs<std::string>().c_str());
                break;
        }
    }
}

void SettingsManager::load()
{

}