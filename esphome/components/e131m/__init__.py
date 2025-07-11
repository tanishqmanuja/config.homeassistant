import esphome.codegen as cg
from esphome.components.light.effects import register_monochromatic_effect
from esphome.components.light.types import LightEffect
import esphome.config_validation as cv
from esphome.const import CONF_CHANNELS, CONF_ID, CONF_METHOD, CONF_NAME

DEPENDENCIES = ["network"]

e131m_ns = cg.esphome_ns.namespace("e131m")
E131MonoLightEffect = e131m_ns.class_(
    "E131MonoLightEffect", LightEffect
)
E131MComponent = e131m_ns.class_("E131Component", cg.Component)

METHODS = {"UNICAST": e131m_ns.E131_UNICAST, "MULTICAST": e131m_ns.E131_MULTICAST}

CHANNELS = {
    "MONO": e131m_ns.E131_MONO,
    "RGB": e131m_ns.E131_RGB,
}

CONF_UNIVERSE = "universe"
CONF_E131M_ID = "e131m_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(E131MComponent),
        cv.Optional(CONF_METHOD, default="MULTICAST"): cv.one_of(*METHODS, upper=True),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_method(METHODS[config[CONF_METHOD]]))
    cg.add_library("https://github.com/tanishqmanuja/ESPAsyncE131",None)


@register_monochromatic_effect(
    "e131m",
    E131MonoLightEffect,
    "E1.31",
    {
        cv.GenerateID(CONF_E131M_ID): cv.use_id(E131MComponent),
        cv.Required(CONF_UNIVERSE): cv.int_range(min=1, max=512),
        cv.Optional(CONF_CHANNELS, default="MONO"): cv.one_of(*CHANNELS, upper=True),
    },
)
async def e131m_light_effect_to_code(config, effect_id):
    parent = await cg.get_variable(config[CONF_E131M_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_first_universe(config[CONF_UNIVERSE]))
    cg.add(effect.set_channels(CHANNELS[config[CONF_CHANNELS]]))
    cg.add(effect.set_e131(parent))

    return effect

