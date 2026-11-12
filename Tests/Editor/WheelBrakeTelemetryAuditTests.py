"""Synthetic model log fixtures, not engine or installed-game evidence."""
from pathlib import Path
import runpy
import unittest

ROOT=Path(__file__).resolve().parents[2]
M=runpy.run_path(str(ROOT/'scripts/AnalyzeWheelBrakeTelemetry.py'))
parse_record=M['parse_record'];analyze=M['analyze']


def line(receipt=1,actor=42,driver='player',mask=15):
    fields={'v':'1','driver':driver,'source':'modeled','actor_id':str(actor),
        'receipt':str(receipt),'steps':'10','iterations':'1440','vehicle':'0',
        'active_mask':str(mask),'dt_s':'0.016667'}
    for i,c in enumerate(('fl','fr','rl','rr')):
        active=bool(mask&(1<<i))
        fields.update({f'road_{c}_cmps':'3000.0',f'omega_{c}':'95.0' if active else '0.0',
            f'slip_{c}':'-0.08' if active else '0.0',f'pressure_{c}':'0.5' if active else '0.0',
            f'normal_est_{c}_n':'2695.0' if active else '0.0',
            f'command_{c}_n':'-2000.0' if active else '0.0'})
    return 'LogTemp: Display: LATEBRAKE_WHEEL_BRAKES '+' '.join(f'{k}={v}' for k,v in fields.items())


class WheelBrakeTelemetryAuditTests(unittest.TestCase):
    def test_valid_records_are_explicitly_model_only_and_actor_scoped(self):
        report=analyze(['unrelated',line(),line(3),line(1,99,'autonomous')],require_player=True)
        self.assertEqual(report['records'],3)
        self.assertEqual(report['actors'],2)
        self.assertFalse(report['unreal_runtime_verified'])
        self.assertLess(report['sampled_command_work_joules'],0.0)

    def test_missing_unknown_and_duplicate_fields_fail_closed(self):
        for text in (line().replace('dt_s=0.016667',''),line()+' extra=1',line()+' steps=10'):
            with self.subTest(text=text),self.assertRaises(ValueError):parse_record(text)

    def test_nonfinite_and_invalid_domains_fail_closed(self):
        for old,new in [('omega_fl=95.0','omega_fl=nan'),('pressure_fl=0.5','pressure_fl=1.1'),
                        ('source=modeled','source=measured'),('v=1','v=2'),
                        ('iterations=1440','iterations=1441'),('dt_s=0.016667','dt_s=0.2'),
                        ('receipt=1','receipt=0')]:
            with self.subTest(new=new),self.assertRaises(ValueError):parse_record(line().replace(old,new))

    def test_positive_brake_work_and_unloaded_force_are_rejected(self):
        with self.assertRaises(ValueError):parse_record(line().replace('command_fl_n=-2000.0','command_fl_n=2000.0'))
        with self.assertRaises(ValueError):parse_record(line().replace('normal_est_fl_n=2695.0','normal_est_fl_n=0.0'))
        with self.assertRaises(ValueError):parse_record(line(mask=14).replace('omega_fl=0.0','omega_fl=1.0'))

    def test_stale_receipts_and_wrong_driver_scope_are_rejected(self):
        with self.assertRaises(ValueError):analyze([line(2),line(2)])
        with self.assertRaises(ValueError):analyze([line(2),line(1)])
        with self.assertRaises(ValueError):analyze([line(driver='autonomous')],require_player=True)
        with self.assertRaises(ValueError):analyze(['no observations'])

    def test_inactive_channels_are_not_counted_as_pressure_modulation(self):
        report=analyze([line(mask=12)])
        self.assertNotIn('fl',report['active_channel_samples'])
        self.assertEqual(report['modeled_pressure_reduction_samples'],{'rl':1,'rr':1})


if __name__=='__main__':unittest.main()
