// SkyrimNet_Leashed: TargetMenu "leash" panel for SkyrimNet_SexLab.
// Ships as an MO2 overlay at PrismaUI/views/SkyrimNet_SexLab/panels/leashed.js; the option
// 0700_leashed_panel.json names it in "panelScript" so the SexLab view loads it on first open.
// Uses SexLab view globals: createPulldownCascade, ssListCascade, firePapyrusOption,
// renderPapyrusPanel, eligibleParamActors, outfitFindActor, formIdU32, tmActorParam,
// hidePanel, papyrusQuery, registerTargetPanel, targetActor, playerActor, papyrusSelectedOpt.
(function () {
    let leashUi = {
        subject: 0,
        leashed: 0,
        holder: 0,
        style: 'normally',
        action: 'give to',
        tiePoint: 'floor',
        leashType: 'rope',
        bodyPart: 'neck',
        isLeashed: false,
        statusHolderFormId: 0,
        statusHolderName: ''
    };
    const LEASH_STYLES = ['forcefully', 'normally', 'gently'];
    const LEASH_TIE_POINTS = ['floor', 'left', 'back', 'front', 'right', 'wall'];
    const LEASH_DIST_DEFAULT = 'middle';
    const LEASH_TYPE_DEFAULT = 'rope';
    const LEASH_BODY_DEFAULT = 'neck';
    const LEASH_TYPES = ['chain', 'rope', 'magic'];
    const LEASH_BODY_PARTS = ['neck', 'wrists', 'waist'];
    // Leash.esm meshes per body part (Leashed LeashIdFor): wrists = chain, waist = rope.
    const LEASH_BODY_KINDS = { neck: ['chain', 'rope', 'magic'], wrists: ['chain'], waist: ['rope'] };
    function leashBodyValid(body, kind) {
        return (LEASH_BODY_KINDS[body] || []).includes(kind);
    }

    function leashSeedAction() {
        if (!leashUi.isLeashed && leashUi.action === 'unleash')
            leashUi.action = 'give to';
    }

    function leashRequestStatus(formId) {
        const fid = formIdU32(formId);
        if (!fid) {
            leashUi.isLeashed = false;
            leashUi.statusHolderFormId = 0;
            leashUi.statusHolderName = '';
            return;
        }
        if (typeof papyrusQuery !== 'function')
            return;
        papyrusQuery('SkyrimNet_Leashed_Actions', 'LeashStatus', fid)
            .then((value) => leashStatusResult(fid, value));
    }

    // SkyrimNet_Leashed_Actions.LeashStatus: '' = not leashed, 'world' = tied to a point,
    // '<holderFormId>|<holderName>' = held. null = query failed (keep current state).
    function leashStatusResult(fid, value) {
        if (value === null || fid !== formIdU32(leashUi.leashed))
            return;
        const v = String(value || '');
        leashUi.isLeashed = v !== '';
        leashUi.statusHolderFormId = 0;
        leashUi.statusHolderName = '';
        const bar = v.indexOf('|');
        if (bar > 0) {
            leashUi.statusHolderFormId = formIdU32(v.slice(0, bar));
            leashUi.statusHolderName = v.slice(bar + 1);
        }
        leashSeedAction();
        if (papyrusSelectedOpt && papyrusSelectedOpt.panel === 'leash')
            renderPapyrusPanel();
    }

    function leashFireStart(opt) {
        const subject = tmActorParam(leashUi.subject);
        const leashed = tmActorParam(leashUi.leashed);
        const holder = tmActorParam(leashUi.holder);
        const style = leashUi.style || 'normally';
        const dist = LEASH_DIST_DEFAULT;
        const kind = leashUi.leashType || LEASH_TYPE_DEFAULT;
        const body = leashUi.bodyPart || LEASH_BODY_DEFAULT;
        let fn = 'LeashedToHolder';
        let mapping = [
            { type: 'speaker', name: 'subject' },
            { type: 'target', name: 'leashed' },
            { type: 'speaker', name: 'holder' },
            { type: 'dynamic', name: 'style' },
            { type: 'dynamic', name: 'leashDistance' },
            { type: 'dynamic', name: 'leashType' },
            { type: 'dynamic', name: 'body_part' }
        ];
        let parameters = {
            subject: subject,
            leashed: leashed,
            holder: holder,
            style: style,
            leashDistance: dist,
            leashType: kind,
            body_part: body,
            closeWebUI: true
        };
        if (leashUi.action === 'unleash') {
            if (formIdU32(leashUi.subject) === formIdU32(leashUi.leashed)) {
                fn = 'UnleashSpeakerExecute';
                mapping = [{ type: 'speaker', name: 'subject' }];
                parameters = { subject: subject, closeWebUI: true };
            } else {
                fn = 'UnleashTargetExecute';
                mapping = [
                    { type: 'speaker', name: 'subject' },
                    { type: 'target', name: 'target' }
                ];
                parameters = { subject: subject, target: leashed, closeWebUI: true };
            }
        } else if (leashUi.action === 'tie to') {
            fn = 'LeashedToTiePoint';
            mapping = [
                { type: 'speaker', name: 'subject' },
                { type: 'target', name: 'leashed' },
                { type: 'dynamic', name: 'style' },
                { type: 'dynamic', name: 'leashDistance' },
                { type: 'dynamic', name: 'leashType' },
                { type: 'dynamic', name: 'body_part' },
                { type: 'dynamic', name: 'tiePoint' }
            ];
            parameters = {
                subject: subject,
                leashed: leashed,
                style: style,
                leashDistance: dist,
                leashType: kind,
                body_part: body,
                tiePoint: leashUi.tiePoint || 'floor',
                closeWebUI: true
            };
        } else if (leashUi.isLeashed) {
            if (formIdU32(leashUi.holder) === formIdU32(leashUi.subject)) {
                fn = 'TakeLeash';
                mapping = [
                    { type: 'speaker', name: 'subject' },
                    { type: 'target', name: 'leashed' }
                ];
                parameters = { subject: subject, leashed: leashed, closeWebUI: true };
            } else {
                fn = 'GiveLeash';
                mapping = [
                    { type: 'speaker', name: 'subject' },
                    { type: 'target', name: 'leashed' },
                    { type: 'target', name: 'receiver' }
                ];
                parameters = {
                    subject: subject,
                    leashed: leashed,
                    receiver: holder,
                    closeWebUI: true
                };
            }
        }
        firePapyrusOption(opt, Object.assign({
            executionFunctionName: fn,
            parameterMapping: mapping
        }, parameters));
        if (typeof hidePanel === 'function') {
            hidePanel('target_menu_panel');
            hidePanel('control_panel');
        }
    }

    // Actor pulldown_cascade over a pool (no "none" row, unlike ssFidCascade).
    function leashActorCascade(key, caption, pool, currentFid, onPick) {
        const cur = formIdU32(currentFid);
        const hit = cur ? pool.find(a => formIdU32(a.formId) === cur) : null;
        return createPulldownCascade({
            key: key,
            caption: caption,
            display: hit ? hit.label : '—',
            tree: pool.map(a => ({
                label: a.label,
                value: formIdU32(a.formId),
                current: formIdU32(a.formId) === cur
            })),
            onPick: (node) => onPick(node.value)
        });
    }

    // ssListCascade with per-value greying (disabled leaves are unclickable).
    function leashListCascade(key, caption, values, current, isDisabled, onPick) {
        return createPulldownCascade({
            key: key,
            caption: caption,
            display: current,
            tree: values.map(v => ({
                label: v,
                value: v,
                current: v === current,
                disabled: isDisabled(v)
            })),
            onPick: (node) => onPick(node.value)
        });
    }

    function leashCanStart() {
        return !!leashUi.subject && !!leashUi.leashed
            && (leashUi.action !== 'give to' || (leashUi.holder && leashUi.holder !== leashUi.leashed));
    }

    // action row = verb → object tree (like the scene-start method row); a leaf sets
    // action + holder/tiePoint and starts at once.
    function leashActionCascade(opt, holderPool) {
        const actionOf = (v) => leashUi.action === v;
        const tieNodes = LEASH_TIE_POINTS.map(t => ({
            label: t,
            value: { action: 'tie to', tiePoint: t },
            current: actionOf('tie to') && leashUi.tiePoint === t
        }));
        const holderNodes = holderPool.map(a => ({
            label: a.label,
            value: { action: 'give to', holder: formIdU32(a.formId) },
            current: actionOf('give to') && formIdU32(a.formId) === formIdU32(leashUi.holder)
        }));
        const tree = [];
        if (leashUi.isLeashed)
            tree.push({ label: 'unleash', value: { action: 'unleash' }, current: actionOf('unleash') });
        tree.push({ label: 'tie to', current: actionOf('tie to'), children: tieNodes });
        tree.push({ label: 'give to', current: actionOf('give to'), children: holderNodes });

        let display = leashUi.action;
        if (actionOf('tie to')) {
            display = 'tie to: ' + leashUi.tiePoint;
        } else if (actionOf('give to')) {
            const h = holderPool.find(a => formIdU32(a.formId) === formIdU32(leashUi.holder));
            display = 'give to: ' + (h ? h.label : '—');
        }
        return createPulldownCascade({
            key: 'leash_action',
            caption: 'action',
            display: display,
            tree: tree,
            onPick: (node) => {
                const v = node.value || {};
                leashUi.action = v.action;
                if (v.tiePoint) leashUi.tiePoint = v.tiePoint;
                if (v.holder) leashUi.holder = v.holder;
                if (leashCanStart()) leashFireStart(opt);
                else renderPapyrusPanel();
            }
        });
    }

    // Layout (pulldown_cascade, like renderSceneStartPanelBody): [Start] style /
    // subject / leashed / (is leashed by | is tied to) / action.
    function renderLeashPanelBody(body, opt) {
        const pool = eligibleParamActors();
        let subject = outfitFindActor(pool, leashUi.subject)
            || outfitFindActor(pool, playerActor && playerActor.formId)
            || pool[0]
            || null;
        let leashed = outfitFindActor(pool, leashUi.leashed)
            || outfitFindActor(pool, targetActor && targetActor.formId)
            || pool[0]
            || null;
        if (subject) leashUi.subject = formIdU32(subject.formId);
        if (leashed) leashUi.leashed = formIdU32(leashed.formId);
        if (!LEASH_STYLES.includes(leashUi.style))
            leashUi.style = 'normally';
        if (!LEASH_TIE_POINTS.includes(leashUi.tiePoint))
            leashUi.tiePoint = 'floor';
        if (!LEASH_TYPES.includes(leashUi.leashType))
            leashUi.leashType = LEASH_TYPE_DEFAULT;
        if (!leashBodyValid(leashUi.bodyPart, leashUi.leashType))
            leashUi.bodyPart = LEASH_BODY_DEFAULT;
        leashSeedAction();

        const holderPool = pool.filter(a => formIdU32(a.formId) !== formIdU32(leashUi.leashed));
        const holder = outfitFindActor(holderPool, leashUi.holder)
            || outfitFindActor(holderPool, leashUi.subject)
            || holderPool[0]
            || null;
        leashUi.holder = holder ? formIdU32(holder.formId) : 0;

        const bar = document.createElement('div');
        bar.className = 'bottom-bar top-bar';
        const startBtn = document.createElement('div');
        startBtn.className = 'action-btn';
        startBtn.textContent = 'Start';
        if (!leashCanStart()) startBtn.classList.add('disabled');
        startBtn.onclick = () => {
            if (startBtn.classList.contains('disabled')) return;
            leashFireStart(opt);
        };
        bar.appendChild(startBtn);
        bar.appendChild(ssListCascade('leash_style', 'style', LEASH_STYLES, leashUi.style, (v) => {
            leashUi.style = v;
            renderPapyrusPanel();
        }));
        body.appendChild(bar);

        const fields = document.createElement('div');
        fields.className = 'labeled-fields';
        const addField = (labelText, node) => {
            const field = document.createElement('div');
            field.className = 'dyn-field';
            const lab = document.createElement('label');
            lab.textContent = labelText || ' ';
            field.appendChild(lab);
            if (node) field.appendChild(node);
            fields.appendChild(field);
        };

        addField('subject', leashActorCascade('leash_subject', 'subject', pool, leashUi.subject, (fid) => {
            leashUi.subject = fid;
            renderPapyrusPanel();
        }));
        addField('leashed', leashActorCascade('leash_leashed', 'leashed', pool, leashUi.leashed, (fid) => {
            leashUi.leashed = fid;
            leashUi.isLeashed = false;
            leashUi.statusHolderFormId = 0;
            leashUi.statusHolderName = '';
            leashRequestStatus(leashUi.leashed);
            renderPapyrusPanel();
        }));
        if (leashUi.isLeashed) {
            const statusVal = document.createElement('span');
            if (leashUi.statusHolderFormId) {
                statusVal.textContent = leashUi.statusHolderName || '?';
                addField('is leashed by', statusVal);
            } else {
                statusVal.textContent = 'world';
                addField('is tied to', statusVal);
            }
        }
        // type/body only reach Papyrus for a new leash or a tie-to; above action
        // because an action leaf starts at once.
        if (!leashUi.isLeashed || leashUi.action === 'tie to') {
            addField('type', ssListCascade('leash_type', 'type', LEASH_TYPES, leashUi.leashType, (v) => {
                leashUi.leashType = v;
                if (!leashBodyValid(leashUi.bodyPart, v))
                    leashUi.bodyPart = LEASH_BODY_DEFAULT;
                renderPapyrusPanel();
            }));
            addField('body', leashListCascade('leash_body', 'body', LEASH_BODY_PARTS, leashUi.bodyPart,
                (b) => !leashBodyValid(b, leashUi.leashType), (v) => {
                    leashUi.bodyPart = v;
                    renderPapyrusPanel();
                }));
        }
        addField('action', leashActionCascade(opt, holderPool));
        body.appendChild(fields);
    }

    function leashOnOpen(opt) {
        const pool = eligibleParamActors();
        if (!outfitFindActor(pool, leashUi.leashed))
            leashUi.leashed = formIdU32(targetActor && targetActor.formId)
                || (pool[0] && pool[0].formId) || 0;
        if (!outfitFindActor(pool, leashUi.subject))
            leashUi.subject = formIdU32(playerActor && playerActor.formId)
                || leashUi.leashed;
        if (!outfitFindActor(pool, leashUi.holder))
            leashUi.holder = leashUi.subject;
        leashRequestStatus(leashUi.leashed);
    }

    registerTargetPanel('leash', {
        render: renderLeashPanelBody,
        onOpen: leashOnOpen,
        skipHeader: true
    });
})();
