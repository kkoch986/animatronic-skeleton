import inquirer from 'inquirer';
import { exec } from 'child_process';
import { readFileSync, writeFileSync } from 'fs';
import { ArgumentParser } from 'argparse';

const NUM_SERVOS = 16;

const
  WRITE_CHANNEL =  0,
  SWEEP_OFFSET  =  0,
  MIN_OFFSET    =  1,
  CENTER_OFFSET =  2,
  MAX_OFFSET    =  3,
  SMOOTH_OFFSET =  4,
  MTYPE_OFFSET  =  5,
  EN_OFFSET     =  6;

const DEF_JAW_INDEX  = 0,
      DEF_PAN_INDEX  = 1,
      DEF_TILT_INDEX = 2,
      DEF_NOD_INDEX  = 3,
      DEF_EYEH_INDEX = 4,
      DEF_EYEV_INDEX = 5;

const
  PROMPT_SET_MIN     = "Set Min",
  PROMPT_SET_SMOOTH  = "Set Smoothing",
  PROMPT_SET_MAX     = "Set Max",
  PROMPT_SET_CENTER  = "Set Center",
  PROMPT_CENTER_INFER   = "Infer Center",
  PROMPT_ENABLE = "Enable Motor",
  PROMPT_DISABLE = "Disable Motor",
  PROMPT_SET_MTYPE = "Set Motor Type",
  PROMPT_NAME_MOTOR = "Set Motor Name (config tool only)",
  PROMPT_CONTINUE = "Write data and continue",
  PROMPT_SAVE_AND_QUIT = "Save and Quit";

function initialChannels() {
  const r = (new Array(512)).fill(0);
  const jawStart  = 1 + (7 * DEF_JAW_INDEX);
  const panStart  = 1 + (7 * DEF_PAN_INDEX);
  const tiltStart = 1 + (7 * DEF_TILT_INDEX);
  const nodStart  = 1 + (7 * DEF_NOD_INDEX);
  const eyehStart = 1 + (7 * DEF_EYEH_INDEX);
  const eyevStart = 1 + (7 * DEF_EYEV_INDEX);
  r[jawStart + MIN_OFFSET]      = 1;
  r[jawStart + CENTER_OFFSET]   = 180;
  r[jawStart + MAX_OFFSET]      = 255;
  r[jawStart + SMOOTH_OFFSET]   = 100;
  r[jawStart + MTYPE_OFFSET]    = 0;
  r[jawStart + EN_OFFSET]       = 255;
  r[panStart + MIN_OFFSET]      = 1;
  r[panStart + CENTER_OFFSET]   = 180;
  r[panStart + MAX_OFFSET]      = 255;
  r[panStart + SMOOTH_OFFSET]   = 32;
  r[panStart + MTYPE_OFFSET]    = 2;
  r[panStart + EN_OFFSET]       = 255;
  r[tiltStart + MIN_OFFSET]      = 40;
  r[tiltStart + CENTER_OFFSET]   = 75;
  r[tiltStart + MAX_OFFSET]      = 127;
  r[tiltStart + SMOOTH_OFFSET]   = 32;
  r[tiltStart + MTYPE_OFFSET]    = 2;
  r[tiltStart + EN_OFFSET]       = 255;
  r[nodStart + MIN_OFFSET]      = 1;
  r[nodStart + CENTER_OFFSET]   = 90;
  r[nodStart + MAX_OFFSET]      = 127;
  r[nodStart + SMOOTH_OFFSET]   = 32;
  r[nodStart + MTYPE_OFFSET]    = 2;
  r[nodStart + EN_OFFSET]       = 255;
  r[eyehStart + MIN_OFFSET]      = 60;
  r[eyehStart + CENTER_OFFSET]   = 100;
  r[eyehStart + MAX_OFFSET]      = 127;
  r[eyehStart + SMOOTH_OFFSET]   = 32;
  r[eyehStart + MTYPE_OFFSET]    = 0;
  r[eyehStart + EN_OFFSET]       = 255;
  r[eyevStart + MIN_OFFSET]      = 120;
  r[eyevStart + CENTER_OFFSET]   = 150;
  r[eyevStart + MAX_OFFSET]      = 180;
  r[eyevStart + SMOOTH_OFFSET]   = 32;
  r[eyevStart + MTYPE_OFFSET]    = 0;
  r[eyevStart + EN_OFFSET]       = 255;
  return r;
}

function setDMX(universe, dmxValues, delay = 1000) {
  return new Promise((resolve, reject) => {
    const cmd = "ola_set_dmx --universe " + universe + " --dmx " + dmxValues.join(",");
    console.log(cmd);
    exec(cmd, (error, stdout, stderr) => {
      if (error) {
        console.warn(error);
        reject(error);
      } else {
        setTimeout(() => resolve(stdout || stderr), delay);
      }
    });
  });
}

function invokeCommandModeValues() {
  return (new Array(512)).fill(0).map((i, j) => (j % 255) + 1);
}

function init(universe) {
  return setDMX(universe, invokeCommandModeValues())
    .then(() => inquirer.prompt([
      {
        name: "ready",
        message: "command mode initialized, turn on the device now and wait for the red light to come on, then press enter",
      },
    ])) 
    .then(() => {
        console.log("writing initial data"  + JSON.stringify(currentDMXData) + " to device");
        return setDMX(universe, currentDMXData).then(() => inquirer.prompt([
          {
            name: "start",
            message: "press enter to start configuration",
          },
        ]))
    });
}

function writeOutputFile() {
  // ask for the file name to write to
  return inquirer.prompt([
    {
      name: "filename",
      message: "enter a filename to write to",
      type: "input",
    },
  ]).then((answers) => {
    const fn = answers["filename"];
    console.log("writing to " + fn);
    writeFileSync(fn, JSON.stringify({"dmx": currentDMXData, "names": motorNames}));
  });
}

function selectMotorType() {
  return inquirer.prompt([
    {
      name: "type",
      message: "Select a motor type:",
      type: "list",
      choices: [
        { name: "SG90", value: 0 },
        { name: "MG90S", value: 1 },
        { name: "MG995", value: 2 },
        { name: "HS805BB", value: 3 },
      ],
    },
  ]);
}

function getValue() {
  return inquirer.prompt([
    {
      name: "value",
      message: "Enter value (0 - 255)",
      type: "int",
    },
  ]);
}

let quit = false;
function generateConfigStep(
  universe,
  motorIndex,
  idxsweep,
  idxmin,
  idxcenter,
  idxmax,
  idxsmooth,
  idxmtype,
  idxen
) {
  return () => new Promise(async (accept, reject) => {
    if (quit) {
      accept();
      return ;
    }
    console.log("enabling sweep");
    currentDMXData[idxsweep] = 255;
    setDMX(universe, currentDMXData);
    while(true) {
      const label = motorNames[motorIndex] || "MOTOR " + motorIndex;
      console.clear();
      console.log("\n\nCONFIGURING " + label + "(" + motorIndex + ")\n");
      console.log("Current data:");
      console.log("MIN:     " + currentDMXData[idxmin]);
      console.log("CENTER:  " + currentDMXData[idxcenter]);
      console.log("MAX:     " + currentDMXData[idxmax]);
      console.log("SMOOTH:  " + currentDMXData[idxsmooth]);
      console.log("MTYPE:   " + currentDMXData[idxmtype]);
      console.log("ENABLED: " + currentDMXData[idxen]);
      const answer = await inquirer.prompt([
        {
          name: "action",
          type: "list",
          loop: true,
          pageSize: 30,
          choices: [
            { name: PROMPT_SET_MIN },
            { name: PROMPT_SET_MAX },
            PROMPT_SET_CENTER,
            PROMPT_CENTER_INFER, 
            PROMPT_SET_SMOOTH,
            new inquirer.Separator(),
            { name: PROMPT_ENABLE, disabled: currentDMXData[idxen] === 255 },
            { name: PROMPT_DISABLE, disabled: currentDMXData[idxen] !== 255 },
            PROMPT_NAME_MOTOR,
            PROMPT_SET_MTYPE,
            new inquirer.Separator(),
            PROMPT_CONTINUE,
            PROMPT_SAVE_AND_QUIT,
            new inquirer.Separator(),
          ],
        },
      ]);
      switch(answer.action) {
        case PROMPT_ENABLE:
          currentDMXData[idxen] = 255;
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_DISABLE:
          currentDMXData[idxen] = 0;
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_SET_MTYPE:
          const mtv = await selectMotorType();
          currentDMXData[idxmtype] = mtv["type"];
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_NAME_MOTOR:
          const nv = await getValue();
          motorNames[motorIndex] = nv["value"];
          break ;
        case PROMPT_SET_SMOOTH:
          const sval = await getValue();
          currentDMXData[idxsmooth] = Math.min(255, Math.max(0, sval["value"]));
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_SET_CENTER:
          const cval = await getValue();
          currentDMXData[idxcenter] = Math.min(255, Math.max(0, cval["value"]));
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_SET_MAX:
          const maxval = await getValue();
          currentDMXData[idxmax] = Math.min(255, Math.max(0, maxval["value"]));
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_SET_MIN:
          const minval = await getValue();
          currentDMXData[idxmin] = Math.min(255, Math.max(0, minval["value"]));
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_CENTER_INFER:
          currentDMXData[idxcenter] = parseInt(Math.round(currentDMXData[idxmin] + (currentDMXData[idxmax] - currentDMXData[idxmin]) / 2.0));
          await setDMX(universe, currentDMXData);
          break ;
        case PROMPT_SAVE_AND_QUIT:
          quit = true;
          // intentional fallthrough to PROMPT_CONTINUE
        case PROMPT_CONTINUE:
          console.log("disabling sweep & writing config");
          currentDMXData[idxsweep] = 0;
          currentDMXData[WRITE_CHANNEL] = 255;
          await setDMX(universe, currentDMXData, 3000);
          currentDMXData[WRITE_CHANNEL] = 0;
          await setDMX(universe, currentDMXData);
          accept();
          return;
        default:
          console.error("unhandled action", answer);
      }
    }
  });
}


// parse the input args
let currentDMXData = initialChannels();
let motorNames = new Array(NUM_SERVOS).fill("").map((i, j) => `SERVO ${j}`);
motorNames[DEF_JAW_INDEX] = "JAW";
motorNames[DEF_PAN_INDEX] = "PAN";
motorNames[DEF_TILT_INDEX] = "TILT";
motorNames[DEF_NOD_INDEX] = "NOD";
motorNames[DEF_EYEH_INDEX] = "EYEH";
motorNames[DEF_EYEV_INDEX] = "EYEV";

const parser = new ArgumentParser({
  description: "command line tool for configuring my dmx skeletons",
  add_help: true,
});
parser.add_argument("-f", "--file", { help: "a file to load the initial DMX data from" });
parser.add_argument("-u", "--universe", { help: "the ola universe to use, can be the ID or the name", default: "aaaa" });

const args = parser.parse_args();
const universe = args["universe"];
const initialFile = args["file"];

console.clear();

if (initialFile) {
  console.log("Reading initial data from " + initialFile);
  const config = JSON.parse(readFileSync(initialFile));
  currentDMXData = config.dmx;
  config.names.forEach((i, j) => motorNames[j] = i);
}

console.log("Welcome to the DMX Skull configuration process.");
console.log("\nMake sure only one skeleton is plugged in to the DMX chain and that all the dip switches are set to ON\n");
console.log("Power on the skeleton, you should see the eyes blink on and off a few times");

let configPromises = [];
for (let i = 1 ; i < NUM_SERVOS * 7 ; i += 7) {
  configPromises.push(generateConfigStep(
    universe,
    Math.floor(i/7),
    i + SWEEP_OFFSET,  
    i + MIN_OFFSET, 
    i + CENTER_OFFSET,
    i + MAX_OFFSET,
    i + SMOOTH_OFFSET,
    i + MTYPE_OFFSET,
    i + EN_OFFSET,
  ));
}

// Enter a loop configuring each step
init(universe)
  .then(() => new Promise(async (accept, reject) => {
    for (const p of configPromises) {
      await p();
    }
    accept();
  }))
  .then(() => writeOutputFile())
  .catch((error) => {
    if (error.isTtyError) {
        // Prompt couldn't be rendered in the current environment
        console.log("unable to render prompt");
      } else {
        // Something else went wrongA
        console.log("Error", error);
      }
  });
