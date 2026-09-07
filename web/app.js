// fheroes2 Save Editor — web UI (vanilla JS, no game graphics).
// Talks to the WASM core (web/wasm_api.cpp) via embind.

( () => {
  "use strict";

  const $ = ( sel ) => document.querySelector( sel );

  const els = {
    fileInput: $( "#file-input" ),
    btnOpen: $( "#btn-open" ),
    btnDownload: $( "#btn-download" ),
    btnDragons: $( "#btn-dragons" ),
    status: $( "#status" ),
    main: $( "#main" ),
    heroList: $( "#hero-list" ),
    heroPanel: $( "#hero-panel" ),
    kingdomPanel: $( "#kingdom-panel" ),
    mapLine: $( "#map-line" ),
  };

  // PlayerColor masks (fheroes2 Color enum) — used for the list dots.
  const COLOR_CSS = {
    1: "#3a6edb",
    2: "#2f9e44",
    4: "#d64545",
    8: "#e0c23a",
    16: "#e07a2a",
    32: "#a55cc0",
  };
  const RACES = [ 1, 2, 4, 8, 16, 32 ]; // Knight..Necromancer (race masks)
  const RESOURCE_NAMES = [ "Wood", "Mercury", "Ore", "Sulfur", "Crystal", "Gems", "Gold" ];
  // Hero list groups, in display order.
  const HERO_GROUPS = [
    { key: "human", title: "Your Kingdoms" },
    { key: "ally", title: "Allies" },
    { key: "enemy", title: "Enemies" },
    { key: "neutral", title: "Not hired" },
  ];

  let api = null;
  const state = {
    fileName: "",
    dirty: false,
    heroIndex: -1,
    heroes: [], // cached list: {name, race, raceName, color}
    quickDragons: false, // the "+5 Black Dragons" one-shot flow
    kingdoms: [], // cached list from kingdomsJson(): {color, status, playerName, castles, towns, resources}
    kingdomColor: 0, // selected kingdom (0 — none; resources hidden)
  };
  const ref = { monsters: [], skills: [], artifacts: [], spells: [], primaryNames: [], levelNames: [] };

  function setStatus( msg, cls ) {
    els.status.textContent = msg || "";
    els.status.className = "status" + ( cls ? " " + cls : "" );
  }

  function esc( s ) {
    return String( s ).replace( /[&<>"']/g, ( c ) => ( { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" } )[ c ] );
  }

  function selectOptions( options, selected ) {
    return options
      .map( ( o ) => `<option value="${ o.value }"${ o.value === selected ? " selected" : "" }${ o.disabled ? " disabled" : "" }>${ esc( o.label ) }</option>` )
      .join( "" );
  }

  // Calls a WASM setter; on error — shows the message and re-renders the hero.
  function apply( fn ) {
    const err = fn();
    if ( err ) {
      setStatus( err, "error" );
      renderHero();
      return false;
    }
    state.dirty = true;
    els.btnDownload.disabled = false;
    setStatus( "Edited — press Download Save.", "ok" );
    return true;
  }

  function nameByteLen( s ) {
    return api ? api.nameCp1251Len( s ) : -1;
  }

  // --- rendering ---

  function heroGroupOf( hero ) {
    if ( hero.color === 0 )
      return "neutral";
    const kingdom = state.kingdoms.find( ( k ) => k.color === hero.color );
    return kingdom ? kingdom.status : "enemy";
  }

  function renderHeroList() {
    // Heroes are grouped by their kingdom's status: your kingdoms → allies →
    // enemies → not hired (neutral). The group header selects the kingdom
    // for the resources block.
    const byGroup = new Map();
    for ( const h of state.heroes )
      ( byGroup.get( heroGroupOf( h ) ) || byGroup.set( heroGroupOf( h ), [] ).get( heroGroupOf( h ) ) ).push( h );
    for ( const list of byGroup.values() )
      list.sort( ( a, b ) => ( RACES.indexOf( a.color ) - RACES.indexOf( b.color ) ) || a.name.localeCompare( b.name ) );

    let html = `<div class="list-title">Heroes (${ state.heroes.length })</div>`;
    for ( const group of HERO_GROUPS ) {
      const heroes = byGroup.get( group.key );
      if ( !heroes || !heroes.length )
        continue;
      const kingdom = state.kingdoms.find( ( k ) => k.color === heroes[0].color );
      const label = group.key === "neutral"
        ? group.title
        : `${ kingdom ? kingdom.playerName || `Kingdom (${ group.title })` : group.title } (${ heroes.length })`;
      const clickable = group.key !== "neutral";
      html += `<button class="hero-group-title${ clickable && heroes[0].color === state.kingdomColor ? " active" : "" }"${ clickable ? ` data-color="${ heroes[0].color }"` : "" }>
        <span class="color-dot" style="background:${ COLOR_CSS[heroes[0].color] || "#555" }"></span>${ esc( label ) }
      </button>`;
      html += heroes.map( ( h, i ) => {
        const idx = state.heroes.indexOf( h );
        return `
        <button class="hero-item${ idx === state.heroIndex ? " active" : "" }" data-i="${ idx }">
          <span class="color-dot" style="background:${ COLOR_CSS[h.color] || "#555" }"></span>${ esc( h.name ) }
          <span class="race">${ esc( h.raceName ) }</span>
        </button>`;
      } ).join( "" );
    }
    els.heroList.innerHTML = html;
    els.heroList.querySelectorAll( ".hero-item" ).forEach( ( b ) => {
      b.addEventListener( "click", () => {
        state.heroIndex = Number( b.dataset.i );
        renderHeroList();
        renderHero();
      } );
    } );
    els.heroList.querySelectorAll( ".hero-group-title" ).forEach( ( b ) => {
      b.addEventListener( "click", () => {
        selectKingdom( Number( b.dataset.color ) );
      } );
    } );
  }

  function renderKingdomPanel() {
    if ( !state.kingdoms.length ) {
      els.kingdomPanel.classList.add( "hidden" );
      return;
    }
    els.kingdomPanel.classList.remove( "hidden" );

    const statusOrder = { human: 0, ally: 1, enemy: 2 };
    const sorted = [ ...state.kingdoms ].sort( ( a, b ) => ( statusOrder[a.status] - statusOrder[b.status] ) || ( RACES.indexOf( a.color ) - RACES.indexOf( b.color ) ) );
    const statusLabel = { human: "Human player", ally: "Ally", enemy: "Enemy (AI)" };
    const selected = state.kingdoms.find( ( k ) => k.color === state.kingdomColor ) || sorted[0];

    const buttons = sorted.map( ( k ) => `
      <button class="kingdom-btn${ k.color === selected.color ? " active" : "" }" data-color="${ k.color }"
        style="background:${ COLOR_CSS[k.color] || "#555" }"
        title="${ esc( k.playerName ? `${ statusLabel[k.status] }: ${ k.playerName }` : statusLabel[k.status] ) }"></button>` ).join( "" );

    const resRows = selected.resources.map( ( value, res ) => `
      <div class="res-row">
        <span class="res-label">${ esc( RESOURCE_NAMES[res] ) }</span>
        <input type="number" min="0" max="999999999" value="${ value }" data-res="${ res }">
      </div>` ).join( "" );

    els.kingdomPanel.innerHTML = `
      <div class="list-title">Kingdom Resources</div>
      <div class="kingdom-switch">${ buttons }</div>
      <p class="kingdom-name">${ esc( selected.playerName || statusLabel[selected.status] ) } — ${ selected.castles } castle(s), ${ selected.towns } town(s)</p>
      ${ resRows }`;

    els.kingdomPanel.querySelectorAll( ".kingdom-btn" ).forEach( ( b ) => {
      b.addEventListener( "click", () => selectKingdom( Number( b.dataset.color ) ) );
    } );
    els.kingdomPanel.querySelectorAll( "input[data-res]" ).forEach( ( input ) => {
      input.addEventListener( "change", () => {
        const err = api.setKingdomResource( selected.color, Number( input.dataset.res ), Number( input.value ) );
        if ( err ) {
          setStatus( err, "error" );
          renderKingdomPanel();
          return;
        }
        state.kingdoms = JSON.parse( api.kingdomsJson() );
        state.dirty = true;
        renderKingdomPanel();
        setStatus( `${ RESOURCE_NAMES[Number( input.dataset.res )] } of the kingdom set.`, "ok" );
      } );
    } );
  }

  function selectKingdom( color ) {
    if ( !state.kingdoms.some( ( k ) => k.color === color ) )
      return;
    state.kingdomColor = color;
    renderKingdomPanel();
    renderHeroList();
  }

  function renderHero() {
    const d = JSON.parse( api.heroDataJson( state.heroIndex ) );
    const baseOk = d.heroBaseParsed;
    const occupied = new Set( d.skills.map( ( s ) => s.id ) );

    // --- header: name / race / portrait ---
    const head = `
      <div class="hero-head">
        <div>
          <h2>${ esc( d.name ) }</h2>
          <div class="hero-sub">${ esc( d.raceName ) } · ${ esc( api.colorName( d.color ) ) } · hero id ${ d.heroId }</div>
        </div>
        <div class="field">
          <label for="f-name">Name</label>
          <input id="f-name" type="text" value="${ esc( d.name ) }" spellcheck="false">
          <span class="name-hint" id="name-hint"></span>
        </div>
        <div class="field">
          <label for="f-race">Race</label>
          <select id="f-race">${ selectOptions( RACES.map( ( r ) => ( { value: r, label: api.raceName( r ) } ) ), d.race ) }</select>
        </div>
        <div class="field">
          <label for="f-portrait">Portrait</label>
          <input id="f-portrait" type="number" min="0" max="72" value="${ d.portrait }">
        </div>
      </div>`;

    // --- primary skills ---
    const primaryValues = [ d.primary.attack, d.primary.defense, d.primary.power, d.primary.knowledge ];
    const primary = `
      <div class="panel-section${ baseOk ? "" : " disabled" }">
        <h3>Primary Skills</h3>
        ${ baseOk ? `<div class="grid primary-grid">${ ref.primaryNames
            .map( ( n, i ) => `<div class="cell"><label>${ esc( n ) }</label><input type="number" min="0" max="99" value="${ primaryValues[i] }" data-prim="${ i }"></div>` )
            .join( "" ) }</div>` : `<span class="locked-note">Hero record not recognized in this save — editing is disabled.</span>` }
      </div>`;

    // --- experience / spell points ---
    const exp = `
      <div class="panel-section">
        <h3>Experience · Spell Points</h3>
        <div class="grid exp-grid">
          <div class="cell"><label for="f-exp">Experience</label><input id="f-exp" type="number" min="0" max="4294967295" value="${ d.experience }"></div>
          <div class="cell"><label for="f-level">Level (derived)</label><input id="f-level" type="number" value="${ d.level }" disabled></div>
          <div class="cell"><label for="f-mana">Spell Points</label>${ baseOk ? `<input id="f-mana" type="number" min="0" max="9999" value="${ d.spellPoints }">` : `<input id="f-mana" type="number" disabled>` }</div>
        </div>
      </div>`;

    // --- army ---
    const monsterOptions = [ { value: 0, label: "(empty)" }, ...ref.monsters.map( ( m ) => ( { value: m.id, label: m.name } ) ) ];
    const army = `
      <div class="panel-section">
        <h3>Army</h3>
        ${ d.army
          .map(
            ( s, i ) => `
          <div class="army-row">
            <span class="slot-num">${ i + 1 }</span>
            <select data-army="${ i }">${ selectOptions( monsterOptions, s.monsterId ) }</select>
            <input type="number" min="0" max="999999" value="${ s.count }" data-army-count="${ i }">
          </div>`
          )
          .join( "" ) }
      </div>`;

    // --- secondary skills ---
    const rowCount = Math.min( 8, d.skills.length + 1 );
    let skillRows = "";
    for ( let i = 0; i < rowCount; ++i ) {
      const cur = i < d.skills.length ? d.skills[i] : null;
      const opts = [ { value: 0, label: "(none)" } ];
      for ( const s of ref.skills ) {
        if ( occupied.has( s.id ) && ( !cur || cur.id !== s.id ) )
          continue;
        opts.push( { value: s.id, label: s.name } );
      }
      const levelOpts = ref.levelNames.map( ( n, li ) => ( { value: li + 1, label: n } ) );
      skillRows += `
        <div class="skill-row">
          <select data-skill="${ i }">${ selectOptions( opts, cur ? cur.id : 0 ) }</select>
          <select class="level-select" data-skill-level="${ i }"${ cur ? "" : " disabled" }>${ selectOptions( levelOpts, cur ? cur.level : 1 ) }</select>
        </div>`;
    }
    const skills = `
      <div class="panel-section">
        <h3>Secondary Skills (${ d.skills.length }/8)</h3>
        ${ skillRows }
      </div>`;

    // --- artifacts ---
    const artifactOptions = [ { value: 0, label: "(empty)" }, ...ref.artifacts.map( ( a ) => ( { value: a.id, label: a.name } ) ) ];
    const artifacts = `
      <div class="panel-section${ baseOk ? "" : " disabled" }">
        <h3>Artifacts (${ d.artifactCount }/14)</h3>
        ${ baseOk ? `<div class="grid artifact-grid">${ d.artifacts
            .map( ( a, i ) => `<select data-artifact="${ i }">${ selectOptions( artifactOptions, a.id ) }</select>` )
            .join( "" ) }</div>` : `<span class="locked-note">Hero record not recognized in this save — editing is disabled.</span>` }
      </div>`;

    // --- spell book ---
    const inBook = new Set( d.spells );
    const spells = `
      <div class="panel-section${ baseOk ? "" : " disabled" }">
        <h3>Spell Book (${ d.spells.length }/65)</h3>
        ${ baseOk ? `<div class="grid spell-grid">${ ref.spells
            .map( ( s ) => `<label class="spell-cell"><input type="checkbox" data-spell="${ s.id }"${ inBook.has( s.id ) ? " checked" : "" }>${ esc( s.name ) }<span class="spell-level">lvl ${ s.level }</span></label>` )
            .join( "" ) }</div>` : `<span class="locked-note">Hero record not recognized in this save — editing is disabled.</span>` }
      </div>`;

    els.heroPanel.innerHTML = head + primary + exp + army + skills + artifacts + spells;
    wireHeroPanel( d );
  }

  function wireHeroPanel( d ) {
    const idx = state.heroIndex;

    // Name: the cp1251 byte length must stay equal to the original.
    const nameInput = $( "#f-name" );
    const nameHint = $( "#name-hint" );
    const updateNameHint = () => {
      const len = nameByteLen( nameInput.value );
      if ( len < 0 ) {
        nameHint.textContent = "Contains characters outside the save's codepage";
        nameHint.className = "name-hint bad";
      }
      else {
        nameHint.textContent = `${ len }/${ d.nameLen } bytes — the byte length must stay the same`;
        nameHint.className = "name-hint" + ( len !== d.nameLen ? " bad" : "" );
      }
    };
    updateNameHint();
    nameInput.addEventListener( "input", updateNameHint );
    const commitName = () => {
      if ( nameByteLen( nameInput.value ) !== d.nameLen ) {
        setStatus( "The name must keep the original byte length.", "error" );
        return;
      }
      if ( nameInput.value === d.name )
        return;
      if ( apply( () => api.setName( idx, nameInput.value ) ) ) {
        d.name = nameInput.value;
        d.nameLen = nameByteLen( nameInput.value );
        updateNameHint();
        document.querySelector( ".hero-panel h2" ).textContent = d.name;
        state.heroes[idx].name = d.name;
        renderHeroList();
      }
    };
    nameInput.addEventListener( "change", commitName );
    nameInput.addEventListener( "keydown", ( e ) => { if ( e.key === "Enter" ) { e.preventDefault(); commitName(); } } );

    $( "#f-race" ).addEventListener( "change", ( e ) => { if ( apply( () => api.setRace( idx, Number( e.target.value ) ) ) ) d.race = Number( e.target.value ); } );
    $( "#f-portrait" ).addEventListener( "change", ( e ) => { if ( apply( () => api.setPortrait( idx, Number( e.target.value ) ) ) ) d.portrait = Number( e.target.value ); } );

    document.querySelectorAll( "[data-prim]" ).forEach( ( inp ) => {
      inp.addEventListener( "change", () => apply( () => api.setPrimarySkill( idx, Number( inp.dataset.prim ), Number( inp.value ) ) ) );
    } );

    $( "#f-exp" ).addEventListener( "change", ( e ) => {
      const v = Math.max( 0, Math.floor( Number( e.target.value ) || 0 ) );
      if ( apply( () => api.setExperience( idx, v ) ) ) d.experience = v;
    } );
    $( "#f-mana" ).addEventListener( "change", ( e ) => { if ( apply( () => api.setSpellPoints( idx, Number( e.target.value ) ) ) ) d.spellPoints = Number( e.target.value ); } );

    document.querySelectorAll( "[data-army]" ).forEach( ( sel ) => {
      sel.addEventListener( "change", () => {
        const slot = Number( sel.dataset.army );
        const id = Number( sel.value );
        const countInput = document.querySelector( `[data-army-count="${ slot }"]` );
        const count = Math.max( 0, Math.floor( Number( countInput.value ) || 0 ) );
        apply( () => api.setSlot( idx, slot, id, id === 0 ? 0 : ( count || 1 ) ) );
      } );
    } );
    document.querySelectorAll( "[data-army-count]" ).forEach( ( inp ) => {
      inp.addEventListener( "change", () => {
        const slot = Number( inp.dataset.armyCount );
        const sel = document.querySelector( `[data-army="${ slot }"]` );
        const id = Number( sel.value );
        const count = Math.max( 0, Math.floor( Number( inp.value ) || 0 ) );
        if ( id === 0 && count !== 0 )
          return;
        apply( () => api.setSlot( idx, slot, id, count ) );
      } );
    } );

    document.querySelectorAll( "[data-skill]" ).forEach( ( sel ) => {
      sel.addEventListener( "change", () => {
        const i = Number( sel.dataset.skill );
        const id = Number( sel.value );
        const levelSel = document.querySelector( `[data-skill-level="${ i }"]` );
        const level = Number( levelSel.value );
        if ( apply( () => api.setSecondarySkill( idx, i, id, id === 0 ? 0 : level ) ) ) {
          if ( id === 0 )
            d.skills.splice( i, 1 );
          else
            d.skills[i] = { id, level };
          renderHero();
        }
      } );
    } );
    document.querySelectorAll( "[data-skill-level]" ).forEach( ( sel ) => {
      sel.addEventListener( "change", () => {
        const i = Number( sel.dataset.skillLevel );
        const skillSel = document.querySelector( `[data-skill="${ i }"]` );
        const id = Number( skillSel.value );
        const level = Number( sel.value );
        if ( apply( () => api.setSecondarySkill( idx, i, id, level ) ) )
          d.skills[i].level = level;
      } );
    } );

    document.querySelectorAll( "[data-artifact]" ).forEach( ( sel ) => {
      sel.addEventListener( "change", () => apply( () => api.setArtifact( idx, Number( sel.dataset.artifact ), Number( sel.value ) ) ) );
    } );

    document.querySelectorAll( "[data-spell]" ).forEach( ( cb ) => {
      cb.addEventListener( "change", () => {
        const ids = [];
        document.querySelectorAll( "[data-spell]:checked" ).forEach( ( c ) => ids.push( Number( c.dataset.spell ) ) );
        if ( apply( () => api.setSpells( idx, ids ) ) )
          d.spells = ids;
      } );
    } );
  }

  // --- open / download ---

  // The hero selected when the save opens: the first hero of a human kingdom
  // (fallback — the first hired hero, then the first record).
  function pickInitialHeroIndex() {
    const humanColors = state.kingdoms.filter( ( k ) => k.status === "human" ).map( ( k ) => k.color );
    const idx = state.heroes.findIndex( ( h ) => h.color !== 0 && humanColors.includes( h.color ) );
    if ( idx >= 0 )
      return idx;
    const hired = state.heroes.findIndex( ( h ) => h.color !== 0 );
    return hired >= 0 ? hired : ( state.heroes.length ? 0 : -1 );
  }

  function loadHeroList() {
    state.heroes = [];
    const count = api.heroCount();
    for ( let i = 0; i < count; ++i ) {
      const d = JSON.parse( api.heroDataJson( i ) );
      state.heroes.push( { name: d.name, race: d.race, raceName: d.raceName, color: d.color } );
    }
    state.heroIndex = pickInitialHeroIndex();
    renderHeroList();
  }

  function renderMapLine() {
    const m = JSON.parse( api.mapInfoJson() );
    const d = ( n ) => String( n ).padStart( 2, "0" );
    els.mapLine.textContent = `${ m.name || state.fileName } — month ${ m.worldMonth }, week ${ m.worldWeek }, day ${ d( m.worldDay ) } · ${ m.width }×${ m.height } · format ${ m.formatVersion } · file ${ state.fileName }`;
  }

  async function openFile( file ) {
    try {
      const buf = new Uint8Array( await file.arrayBuffer() );
      const err = api.openSave( file.name, buf );
      if ( err ) {
        setStatus( err, "error" );
        return;
      }
      state.fileName = file.name;
      state.dirty = false;
      state.kingdoms = JSON.parse( api.kingdomsJson() );
      const human = state.kingdoms.find( ( k ) => k.status === "human" );
      state.kingdomColor = ( human || state.kingdoms[0] || { color: 0 } ).color;
      els.btnDownload.disabled = false;
      els.main.classList.remove( "hidden" );
      els.mapLine.classList.remove( "hidden" );
      loadHeroList();
      renderMapLine();
      renderKingdomPanel();
      renderHero();
      setStatus( `Opened ${ file.name } (${ buf.length.toLocaleString() } bytes).`, "ok" );
    }
    catch ( e ) {
      setStatus( String( e ), "error" );
    }
  }

  function download() {
    const bytes = api.saveBytes();
    const blob = new Blob( [ bytes ], { type: "application/octet-stream" } );
    const url = URL.createObjectURL( blob );
    const a = document.createElement( "a" );
    a.href = url;
    a.download = state.fileName || "save.sav";
    document.body.appendChild( a );
    a.click();
    a.remove();
    setTimeout( () => URL.revokeObjectURL( url ), 2000 );
    setStatus( `Downloaded ${ a.download } (${ bytes.length.toLocaleString() } bytes).`, "ok" );
  }

  // The "+5 Black Dragons" one-shot flow: open a save, add 5 Black Dragons to
  // the human player's first hero and immediately download the edited file.
  async function quickAddDragons( file ) {
    try {
      const buf = new Uint8Array( await file.arrayBuffer() );
      const err = api.openSave( file.name, buf );
      if ( err ) {
        setStatus( err, "error" );
        return;
      }
      const result = JSON.parse( api.quickAddDragons() );
      if ( !result.ok ) {
        setStatus( result.error || "Failed to add dragons.", "error" );
        return;
      }
      state.fileName = file.name.replace( /(\.sav[a-z]*)$/i, "-5dragons$1" ) || `${ file.name }-5dragons`;
      const bytes = api.saveBytes();
      const blob = new Blob( [ bytes ], { type: "application/octet-stream" } );
      const url = URL.createObjectURL( blob );
      const a = document.createElement( "a" );
      a.href = url;
      a.download = state.fileName;
      document.body.appendChild( a );
      a.click();
      a.remove();
      setTimeout( () => URL.revokeObjectURL( url ), 2000 );
      setStatus( `Added 5 Black Dragons to ${ result.hero || "the hero" } and downloaded ${ a.download }.`, "ok" );
    }
    catch ( e ) {
      setStatus( String( e ), "error" );
    }
  }

  // --- init ---

  function initTheme() {
    const themeToggle = $( "#theme-toggle" );
    const themeLabel = $( "#theme-toggle-label" );
    const applyTheme = ( theme ) => {
      document.documentElement.setAttribute( "data-theme", theme );
      const dark = theme === "dark";
      themeLabel.textContent = dark ? "😇 Good" : "😈 Evil";
      themeToggle.setAttribute( "aria-pressed", String( dark ) );
    };
    applyTheme( localStorage.getItem( "fh2theme" ) || "dark" );
    themeToggle.addEventListener( "click", () => {
      const next = document.documentElement.getAttribute( "data-theme" ) === "dark" ? "light" : "dark";
      localStorage.setItem( "fh2theme", next );
      applyTheme( next );
    } );
  }

  async function init() {
    initTheme();
    try {
      api = await createFh2Module();
    }
    catch ( e ) {
      setStatus( "Failed to load the WASM core: " + e, "error" );
      return;
    }
    ref.monsters = JSON.parse( api.monsterListJson() );
    ref.skills = JSON.parse( api.skillListJson() );
    ref.artifacts = JSON.parse( api.artifactListJson() );
    ref.spells = JSON.parse( api.spellListJson() );
    ref.primaryNames = JSON.parse( api.primarySkillNamesJson() );
    ref.levelNames = JSON.parse( api.skillLevelNamesJson() );

    els.btnOpen.disabled = false;
    els.btnOpen.addEventListener( "click", () => els.fileInput.click() );
    els.fileInput.addEventListener( "change", () => {
      if ( els.fileInput.files.length ) {
        if ( state.quickDragons ) {
          quickAddDragons( els.fileInput.files[0] );
          state.quickDragons = false;
        }
        else {
          openFile( els.fileInput.files[0] );
        }
      }
      els.fileInput.value = "";
    } );
    els.btnDownload.addEventListener( "click", download );
    els.btnDragons.disabled = false;
    els.btnDragons.addEventListener( "click", () => {
      state.quickDragons = true;
      els.fileInput.click();
    } );
    setStatus( "Ready. Open an fheroes2 save file.", "" );
  }

  init();
} )();
