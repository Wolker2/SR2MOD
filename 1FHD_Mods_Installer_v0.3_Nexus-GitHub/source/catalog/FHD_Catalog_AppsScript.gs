/**
 * FHD MODS PAK -> JSON catalog, no passwords exposed in installer.
 * Deploy as a Google Apps Script WEB APP (Execute as: me; Access: anyone).
 * This script only reads files from the specified folder.
 * Latest version is picked by numeric version and Drive modified time.
 * Replacing a file with a new Drive revision also works without renaming it.
 */
const ROOT_FOLDER = '16iY_DzJk7X7NxmegMbqYURLl5C6Q8qTd';
const INFO = {
  FHD_Nanotech_Nanogenesis:['Nanotech + Nanogenesis','Эволюционирующие корпуса, новые визуальные эффекты.'],
  FHD_Wormhole:['FHD Wormhole','Корпуса и двигатели «Червоточина», нативная механика.'],
  FHD_Artifacts:['FHD Artifacts','Новые артефакты и игровые способности.'],
  FHD_OnlineChat:['FHD OnlineChat','Общий и личный чат внутри игры; требуется нативный загрузчик.'],
  FHD_QuickQuest:['FHD QuickQuest','Исправления и улучшения заданий.'],
  FHD_Trade:['FHD Trade','Дополнения к торговле.'],
  FHD_QuestMarker:['FHD QuestMarker','Маркеры заданий.'],
  FHD_FindQuest:['FHD FindQuest','Поиск заданий.'],
  FHD_FastNewGame:['FHD FastNewGame','Быстрое начало новой игры.'],
  FHD_UpgradeUnknovn:['FHD UpgradeUnknovn','Расширение улучшений.'],
  FHD_LeftStats:['FHD LeftStats','Дополнительные показатели интерфейса.'],
  FHD_GalaxySearch:['FHD GalaxySearch','Поиск на галактической карте.'],
  FHD_GalaxyZoom:['FHD GalaxyZoom','Масштабирование галактической карты.'],
  FHD_Autopilot:['FHD Autopilot','Дополнительное управление полётом.'],
  FHD_SET:['FHD SET','Набор модификаций FHD SET.'],
  FHD_Hangar:['FHD Hangar','Изменения ангара.'],
  FHD_BIP:['FHD BIP','Набор графических и игровых улучшений.'],
  FHD_LOTOStations:['FHD LOTOStations','Изменения станций.'],
  FHD_BaseBuilder_Paid:['FHD BaseBuilder','Механики строительства баз.'],
  FHD_Animation:['FHD Animation','Дополнительная анимация.'],
  FHD_SpaceBG:['FHD SpaceBG','Фоны космоса.'],
  FHD_FON:['FHD FON','Фоны интерфейса.'],
  FHD_Expedition:['FHD Expedition','Новые экспедиционные возможности.'],
  FHD_AModGalaxy:['FHD AModGalaxyLite','Сборка под AModGalaxyLite.'],
  FHD_AutoUpdater:['FHD AutoUpdater','Обязательный мод автоматических обновлений.']
};
function getId(name) {
  if (/^DevKit/i.test(name)) return null; // editor tools are not game mods
  if (/^FHD[\s_]+ПОД\s+МОД/i.test(name)) return 'FHD_AModGalaxy';
  if (/^FHD_Nanotech.*Nanogenesis/i.test(name)) return 'FHD_Nanotech_Nanogenesis';
  const keys = Object.keys(INFO).sort((a,b)=>b.length-a.length);
  for (const key of keys) {
    if (name.toLowerCase().startsWith(key.toLowerCase())) return key;
  }
  // Unknown FHD archives appear automatically with a generic card.
  const m=name.match(/^(FHD_[A-Za-z][A-Za-z0-9]*)(?:[_ .-]|$)/i);
  return m ? m[1] : null;
}
function getVersion(name) {
  const m=name.match(/(?:^|[ _-])v?([0-9]+(?:[.][0-9]+){1,5})(?:[^0-9]|$)/i);
  return m ? m[1] : 'не указана';
}
function versionScore(ver) {
  if (!/^\d/.test(ver)) return 0;
  return ver.split('.').slice(0,6).reduce((acc,v)=>acc*1000+Number(v),0);
}
function classify(name) {
  const variant=/(?:СТАРЫЙ|Старый|old exe|под старый)/i.test(name)?'old':/(?:НОВЫЙ|new exe|новый ехе)/i.test(name)?'new':'any';
  const channel=/(?:TEST|ALPHA|PREVIEW\s*[0-9]|BETA)/i.test(name)?'test':'stable';
  return {variant,channel};
}
function doGet(e) {
  try {
    const folder=DriveApp.getFolderById(ROOT_FOLDER);
    const files=folder.getFiles();
    const latest={};
    while(files.hasNext()) {
      const f=files.next(),name=f.getName();
      if(!/\.(rar|zip|7z)$/i.test(name)) continue;
      const id=getId(name);
      if(!id) continue;
      const {variant,channel}=classify(name);
      const version=getVersion(name);
      const key=id+'|'+variant+'|'+channel;
      const stamp=f.getLastUpdated().getTime();
      const candidate={
        id:id+(variant==='any'?'':'_'+variant)+(channel==='test'?'__TEST':''),
        title:(INFO[id]?INFO[id][0]:id)+(variant==='old'?' (старый EXE)':variant==='new'?' (новый EXE)':''),
        description:INFO[id]?INFO[id][1]:'Модификация Space Rangers HD из каталога FHD.',
        fileId:f.getId(),fileName:name,size:f.getSize(),version,channel,variant,
        updater:(id==='FHD_AutoUpdater'),modified:f.getLastUpdated().toISOString(),
        _score:versionScore(version),_stamp:stamp
      };
      const old=latest[key];
      if(!old || candidate._score>old._score ||
         (candidate._score===old._score && candidate._stamp>old._stamp)) latest[key]=candidate;
    }
    const mods=Object.values(latest).sort((a,b)=>a.title.localeCompare(b.title,'ru'));
    for(const x of mods){delete x._score;delete x._stamp;}
    return ContentService.createTextOutput(JSON.stringify({
      schema:1,source:'google-drive-live',folderId:ROOT_FOLDER,
      generated:new Date().toISOString(),mods
    })).setMimeType(ContentService.MimeType.JSON);
  } catch(err) {
    return ContentService.createTextOutput(JSON.stringify({error:String(err)}))
      .setMimeType(ContentService.MimeType.JSON);
  }
}
