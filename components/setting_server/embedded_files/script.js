const DAY_PREF = 'd';
const ACT_PREF = 'a'

const LIST_DAY = ['Monday','Thusday','Wednesday','Thursday','Friday','Saturday','Sunday'];
// [formName [type, max limit, min limit,[inputNames],]]
const FORMS_LIST = [
  ['Network',[['text','32','1',['SSID']],['text','32','8',['PWD']]]],
  ['Openweather',[['text','32','1',['City']],['text','32','32',['Key']]]],
  ['Offset',[['number','23','0',['Hour']]]],
  ['Status',[['checkbox',0,,['Openweather Ok','SSID not found',"STA conf. Ok"]]]]
];

const modal = window.document.getElementById('modal');


function getSetting()
{
  fetch('/data?', {
    method:'POST',
    mode: 'no-cors',
    body: null,
  })
  .then((r) => r.json())
  .then((r) => {
    for(const key in r){
      const value = r[key];
      if(key === 'Status'){
        const flags = Number(value);
        [...document.querySelectorAll('[type=checkbox]')].forEach((checkbox, i) =>{
            checkbox.checked = flags&(1<<i);          
          });
      } else {
        const input = document.getElementById(key);
        if(input)
          input.value = value;
      }
    }
   })
  .catch((e) => showModal(e));
}


function createForms()
{
  const containerForms = document.getElementById('settings_forms');
  FORMS_LIST.forEach((form_instance)=>{
    const [formName, inputList] = form_instance;
    const form = document.createElement('form');
    const container = document.createElement('div');
    const fieldset = document.createElement('fieldset');
    const legend = document.createElement('legend');
    legend.innerText = formName;
    fieldset.appendChild(form);
    fieldset.appendChild(legend);
    form.name = formName;
    form.action = '';
    const submit = document.createElement('input');
    submit.value = 'Submit';
    submit.type = 'submit';
    inputList.forEach((inputData)=>{
      const [type, maxLimit, minLimit, inputNames] = inputData;
      inputNames.forEach((inputName, i)=>{
        const label = document.createElement('label');
        label.innerText = inputName+' ';
          const input = document.createElement('input');
          input.type = type;
          input.id = inputName;
          input.name = inputName;
          if(type === 'text'){
            input.value = '';
            input.maxLength = maxLimit;
            input.minLength = minLimit;
            input.placeholder = 'Enter '+ inputName;
          } else if(type == 'checkbox' 
              && i >= maxLimit){
            input.disabled = true;
          } else if(type == 'number'){
            input.max = maxLimit;
            input.min = minLimit;
          }
          label.appendChild(input);
          form.appendChild(label);
        });
        if(type != 'checkbox'){
          form.appendChild(submit);
        }
    });
    container.appendChild(fieldset);
    containerForms.appendChild(container);
});
}

function getInfo()
{
  sendDataForm('info?');
}

function serverExit() 
{
  sendDataForm('close');
  document.getElementById('exit').classList.add('danger');
}


function showModal(str, success) 
{
  modal.innerText = str ? str : ':(';
  modal.style['background-color'] = success === true ? 'green':'red';
  modal.classList.add('show');
  const leftPos = (window.innerWidth - modal.offsetWidth) / 2;
  const topPos = (window.innerHeight - modal.offsetHeight) / 2 + window.scrollY;
  modal.style.right = leftPos + 'px';
  modal.style.top = topPos + 'px';
  setTimeout(() => {
    modal.classList.remove('show');
  },5000);
}


document.body.addEventListener('submit', (e) => {
  e.preventDefault();
  e.stopPropagation();
  sendData(e.target.name);
});


function sendData(formName)
{
  const js = {};
  let data = null;
  let i=0;
  const childsList = document.forms[formName];
  if(childsList){
    for(const child of childsList){
      let value = child.value;
      if(value){
        if(child.type === 'number'){
          data = value;
        } else if(child.type === 'text'){
            if(data == null)
              data = js;
            js[child.name] = value;
        }
      }
    }
    if(data === js){
      data=JSON.stringify(js);
    }
    sendDataForm(formName, data);
  }
}


async function sendDataForm(path, data=""){
  let res = true;
  await fetch('/'+ path, {
    method:'POST',
    mode:'no-cors',
    body:data,
  }).then((r)=>{
      if(!r.ok)
        res=false; 
      return r.text()
    }).then((r) => showModal(r, res))
      .catch((e) => showModal(e, false));
}

createForms();

